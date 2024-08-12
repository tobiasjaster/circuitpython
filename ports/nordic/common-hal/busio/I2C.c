// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Dan Halbert for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2018 Artur Pacholec
// SPDX-FileCopyrightText: Copyright (c) 2017 hathach
// SPDX-FileCopyrightText: Copyright (c) 2016 Sandeep Mistry All right reserved.
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/busio/I2C.h"
#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "supervisor/shared/tick.h"
#include "py/mperrno.h"
#include "py/runtime.h"

#include "nrfx_twim.h"
#include "nrfx_spim.h"
#include "nrf_gpio.h"

// all TWI instances have the same max size
// 16 bits for 840, 10 bits for 810, 8 bits for 832
#define I2C_MAX_XFER_LEN         MIN(((1UL << TWIM0_EASYDMA_MAXCNT_SIZE) - 1), 1024)
#define I2C_TIMEOUT 1000 // 1 second timeout

static twim_peripheral_t twim_peripherals[] = {
    #if NRFX_CHECK(NRFX_TWIM0_ENABLED)
    // SPIM0 and TWIM0 share an address.
    { .twim = NRFX_TWIM_INSTANCE(0),
      .in_use = false,
      .transferring = false,
      .last_event_type = NRFX_TWIM_EVT_DONE},
    #endif
    #if NRFX_CHECK(NRFX_TWIM1_ENABLED)
    // SPIM1 and TWIM1 share an address.
    { .twim = NRFX_TWIM_INSTANCE(1),
      .in_use = false,
      .transferring = false,
      .last_event_type = NRFX_TWIM_EVT_DONE},
    #endif
};

static bool never_reset[MP_ARRAY_SIZE(twim_peripherals)];

void i2c_reset(void) {
    for (size_t i = 0; i < MP_ARRAY_SIZE(twim_peripherals); i++) {
        if (never_reset[i]) {
            continue;
        }
        nrfx_twim_uninit(&twim_peripherals[i].twim);
        twim_peripherals[i].in_use = false;
    }
}

void common_hal_busio_i2c_never_reset(busio_i2c_obj_t *self) {
    for (size_t i = 0; i < MP_ARRAY_SIZE(twim_peripherals); i++) {
        if (self->twim_peripheral == &twim_peripherals[i]) {
            never_reset[i] = true;

            never_reset_pin_number(self->scl_pin_number);
            never_reset_pin_number(self->sda_pin_number);
            break;
        }
    }
}

static uint8_t twi_error_to_mp(const nrfx_err_t err) {
    switch (err) {
        case NRFX_ERROR_DRV_TWI_ERR_ANACK:
            return MP_ENODEV;
        case NRFX_ERROR_BUSY:
            return MP_EBUSY;
        case NRFX_ERROR_INVALID_ADDR:
        case NRFX_ERROR_DRV_TWI_ERR_DNACK:
        case NRFX_ERROR_DRV_TWI_ERR_OVERRUN:
            return MP_EIO;
        case NRFX_ERROR_TIMEOUT:
            return MP_ETIMEDOUT;
        default:
            break;
    }

    return 0;
}

static void twim_event_handler(nrfx_twim_evt_t const *p_event, void *p_context) {
    // this is the callback handler - sets transferring to false and records the most recent event.
    twim_peripheral_t *peripheral = (twim_peripheral_t *)p_context;
    peripheral->last_event_type = p_event->type;
    peripheral->transferring = false;
}

void common_hal_busio_i2c_construct(busio_i2c_obj_t *self, const mcu_pin_obj_t *scl, const mcu_pin_obj_t *sda, uint32_t frequency, uint32_t timeout) {
    if (scl->number == sda->number) {
        raise_ValueError_invalid_pins();
    }

    // Find a free instance.
    self->twim_peripheral = NULL;
    for (size_t i = 0; i < MP_ARRAY_SIZE(twim_peripherals); i++) {
        if (!twim_peripherals[i].in_use) {
            self->twim_peripheral = &twim_peripherals[i];
            // Mark it as in_use later after other validation is finished.
            break;
        }
    }

    if (self->twim_peripheral == NULL) {
        mp_raise_ValueError(MP_ERROR_TEXT("All I2C peripherals are in use"));
    }

    #if CIRCUITPY_REQUIRE_I2C_PULLUPS
    // Test that the pins are in a high state. (Hopefully indicating they are pulled up.)
    nrf_gpio_cfg_input(scl->number, NRF_GPIO_PIN_PULLDOWN);
    nrf_gpio_cfg_input(sda->number, NRF_GPIO_PIN_PULLDOWN);

    common_hal_mcu_delay_us(10);

    nrf_gpio_cfg_input(scl->number, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(sda->number, NRF_GPIO_PIN_NOPULL);

    // We must pull up within 3us to achieve 400khz.
    common_hal_mcu_delay_us(3);

    if (!nrf_gpio_pin_read(sda->number) || !nrf_gpio_pin_read(scl->number)) {
        reset_pin_number(sda->number);
        reset_pin_number(scl->number);
        mp_raise_RuntimeError(MP_ERROR_TEXT("No pull up found on SDA or SCL; check your wiring"));
    }
    #endif

    nrfx_twim_config_t config = NRFX_TWIM_DEFAULT_CONFIG(scl->number, sda->number);

    #if defined(TWIM_FREQUENCY_FREQUENCY_K1000)
    if (frequency >= 1000000) {
        config.frequency = NRF_TWIM_FREQ_1000K;
    } else
    #endif
    if (frequency >= 400000) {
        config.frequency = NRF_TWIM_FREQ_400K;
    } else if (frequency >= 250000) {
        config.frequency = NRF_TWIM_FREQ_250K;
    } else {
        config.frequency = NRF_TWIM_FREQ_100K;
    }

    self->scl_pin_number = scl->number;
    self->sda_pin_number = sda->number;
    claim_pin(sda);
    claim_pin(scl);

    // About to init. If we fail after this point, common_hal_busio_i2c_deinit() will set in_use to false.
    self->twim_peripheral->in_use = true;
    nrfx_err_t err = nrfx_twim_init(&self->twim_peripheral->twim, &config, twim_event_handler, self->twim_peripheral);
    if (err != NRFX_SUCCESS) {
        common_hal_busio_i2c_deinit(self);
        mp_raise_OSError(MP_EIO);
    }

}

bool common_hal_busio_i2c_deinited(busio_i2c_obj_t *self) {
    return self->sda_pin_number == NO_PIN;
}

void common_hal_busio_i2c_deinit(busio_i2c_obj_t *self) {
    if (common_hal_busio_i2c_deinited(self)) {
        return;
    }

    nrfx_twim_uninit(&self->twim_peripheral->twim);

    reset_pin_number(self->sda_pin_number);
    reset_pin_number(self->scl_pin_number);
    self->sda_pin_number = NO_PIN;
    self->scl_pin_number = NO_PIN;

    self->twim_peripheral->in_use = false;
}

// nrfx_twim_tx doesn't support 0-length data so we fall back to the hal API
bool common_hal_busio_i2c_probe(busio_i2c_obj_t *self, uint8_t addr) {
    NRF_TWIM_Type *reg = self->twim_peripheral->twim.p_twim;
    bool found = true;

    nrfx_twim_enable(&self->twim_peripheral->twim);

    nrf_twim_address_set(reg, addr);
    nrf_twim_tx_buffer_set(reg, NULL, 0);

    nrf_twim_task_trigger(reg, NRF_TWIM_TASK_RESUME);

    nrf_twim_task_trigger(reg, NRF_TWIM_TASK_STARTTX);
    while (nrf_twim_event_check(reg, NRF_TWIM_EVENT_TXSTARTED) == 0 &&
           nrf_twim_event_check(reg, NRF_TWIM_EVENT_ERROR) == 0) {
        ;
    }
    nrf_twim_event_clear(reg, NRF_TWIM_EVENT_TXSTARTED);

    nrf_twim_task_trigger(reg, NRF_TWIM_TASK_STOP);
    while (nrf_twim_event_check(reg, NRF_TWIM_EVENT_STOPPED) == 0) {
        ;
    }
    nrf_twim_event_clear(reg, NRF_TWIM_EVENT_STOPPED);

    if (nrf_twim_event_check(reg, NRF_TWIM_EVENT_ERROR)) {
        nrf_twim_event_clear(reg, NRF_TWIM_EVENT_ERROR);

        nrf_twim_errorsrc_get_and_clear(reg);
        found = false;
    }

    nrfx_twim_disable(&self->twim_peripheral->twim);

    return found;
}

bool common_hal_busio_i2c_try_lock(busio_i2c_obj_t *self) {
    bool grabbed_lock = false;
    // NRFX_CRITICAL_SECTION_ENTER();
    if (!self->has_lock) {
        grabbed_lock = true;
        self->has_lock = true;
    }
    // NRFX_CRITICAL_SECTION_EXIT();
    return grabbed_lock;
}

bool common_hal_busio_i2c_has_lock(busio_i2c_obj_t *self) {
    return self->has_lock;
}

void common_hal_busio_i2c_unlock(busio_i2c_obj_t *self) {
    self->has_lock = false;
}

static nrfx_err_t _twim_xfer_with_timeout(busio_i2c_obj_t *self, nrfx_twim_xfer_desc_t const *p_xfer_desc, uint32_t flags) {
    // does non-blocking transfer and raises and exception if it takes longer than I2C_TIMEOUT ms to complete
    uint64_t deadline = supervisor_ticks_ms64() + I2C_TIMEOUT;
    nrfx_err_t err = NRFX_SUCCESS;
    self->twim_peripheral->transferring = true;
    err = nrfx_twim_xfer(&self->twim_peripheral->twim, p_xfer_desc, flags);
    if (err != NRFX_SUCCESS) {
        self->twim_peripheral->transferring = false;
        return err;
    }
    while (self->twim_peripheral->transferring) {
        if (supervisor_ticks_ms64() > deadline) {
            self->twim_peripheral->transferring = false;
            return NRFX_ERROR_TIMEOUT;
        }
    }
    switch (self->twim_peripheral->last_event_type) {
        case NRFX_TWIM_EVT_DONE:       ///< Transfer completed event.
            return NRFX_SUCCESS;
        case NRFX_TWIM_EVT_ADDRESS_NACK: ///< Error event: NACK received after sending the address.
            return NRFX_ERROR_DRV_TWI_ERR_ANACK;
        case NRFX_TWIM_EVT_BUS_ERROR:     ///< Error event: An unexpected transition occurred on the bus.
        case NRFX_TWIM_EVT_DATA_NACK:    ///< Error event: NACK received after sending a data byte.
            return NRFX_ERROR_DRV_TWI_ERR_DNACK;
        case NRFX_TWIM_EVT_OVERRUN:      ///< Error event: The unread data is replaced by new data.
            return NRFX_ERROR_DRV_TWI_ERR_OVERRUN;
        default:                         /// unknown error...
            return NRFX_ERROR_INTERNAL;
    }
}

static uint8_t _common_hal_busio_i2c_write(busio_i2c_obj_t *self, uint16_t addr, const uint8_t *data, size_t len, bool stopBit) {
    if (len == 0) {
        return common_hal_busio_i2c_probe(self, addr) ? 0 : MP_ENODEV;
    }

    nrfx_err_t err = NRFX_SUCCESS;

    nrfx_twim_enable(&self->twim_peripheral->twim);

    // break into MAX_XFER_LEN transaction
    while (len) {
        const size_t xact_len = MIN(len, I2C_MAX_XFER_LEN);
        nrfx_twim_xfer_desc_t xfer_desc = NRFX_TWIM_XFER_DESC_TX(addr, (uint8_t *)data, xact_len);
        uint32_t const flags = (stopBit ? 0 : NRFX_TWIM_FLAG_TX_NO_STOP);

        if (NRFX_SUCCESS != (err = _twim_xfer_with_timeout(self, &xfer_desc, flags))) {
            break;
        }

        len -= xact_len;
        data += xact_len;
    }

    nrfx_twim_disable(&self->twim_peripheral->twim);

    return twi_error_to_mp(err);
}

uint8_t common_hal_busio_i2c_write(busio_i2c_obj_t *self, uint16_t addr, const uint8_t *data, size_t len) {
    return _common_hal_busio_i2c_write(self, addr, data, len, true);
}

uint8_t common_hal_busio_i2c_read(busio_i2c_obj_t *self, uint16_t addr, uint8_t *data, size_t len) {
    if (len == 0) {
        return 0;
    }

    nrfx_err_t err = NRFX_SUCCESS;

    nrfx_twim_enable(&self->twim_peripheral->twim);

    // break into MAX_XFER_LEN transaction
    while (len) {
        const size_t xact_len = MIN(len, I2C_MAX_XFER_LEN);
        nrfx_twim_xfer_desc_t xfer_desc = NRFX_TWIM_XFER_DESC_RX(addr, data, xact_len);

        if (NRFX_SUCCESS != (err = _twim_xfer_with_timeout(self, &xfer_desc, 0))) {
            break;
        }

        len -= xact_len;
        data += xact_len;
    }

    nrfx_twim_disable(&self->twim_peripheral->twim);

    return twi_error_to_mp(err);
}

uint8_t common_hal_busio_i2c_write_read(busio_i2c_obj_t *self, uint16_t addr,
    uint8_t *out_data, size_t out_len, uint8_t *in_data, size_t in_len) {
    uint8_t result = _common_hal_busio_i2c_write(self, addr, out_data, out_len, false);
    if (result != 0) {
        return result;
    }

    return common_hal_busio_i2c_read(self, addr, in_data, in_len);
}
