// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "py/mperrno.h"
#include "py/mphal.h"
#include "shared-bindings/busio/I2C.h"
#include "py/runtime.h"

#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/bitbangio/I2C.h"

#include "src/rp2_common/hardware_gpio/include/hardware/gpio.h"

// Synopsys  DW_apb_i2c  (v2.01)  IP

#define NO_PIN 0xff

// One second
#define BUS_TIMEOUT_US 1000000

static bool never_reset_i2c[2];
static i2c_inst_t *i2c[2] = {i2c0, i2c1};

void reset_i2c(void) {
    for (size_t i = 0; i < 2; i++) {
        if (never_reset_i2c[i]) {
            continue;
        }

        i2c_deinit(i2c[i]);
    }
}

void common_hal_busio_i2c_construct(busio_i2c_obj_t *self,
    const mcu_pin_obj_t *scl, const mcu_pin_obj_t *sda, uint32_t frequency, uint32_t timeout) {
    self->peripheral = NULL;
    // I2C pins have a regular pattern. SCL is always odd and SDA is even. They match up in pairs
    // so we can divide by two to get the instance. This pattern repeats.
    size_t scl_instance = (scl->number / 2) % 2;
    size_t sda_instance = (sda->number / 2) % 2;
    if (scl->number % 2 == 1 && sda->number % 2 == 0 && scl_instance == sda_instance) {
        self->peripheral = i2c[sda_instance];
    }
    if (self->peripheral == NULL) {
        raise_ValueError_invalid_pins();
    }
    if ((i2c_get_hw(self->peripheral)->enable & I2C_IC_ENABLE_ENABLE_BITS) != 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("I2C peripheral in use"));
    }

    mp_arg_validate_int_max(frequency, 1000000, MP_QSTR_frequency);


    #if CIRCUITPY_REQUIRE_I2C_PULLUPS
    // Test that the pins are in a high state. (Hopefully indicating they are pulled up.)
    gpio_set_function(sda->number, GPIO_FUNC_SIO);
    gpio_set_function(scl->number, GPIO_FUNC_SIO);
    gpio_set_dir(sda->number, GPIO_IN);
    gpio_set_dir(scl->number, GPIO_IN);

    gpio_set_pulls(sda->number, false, true);
    gpio_set_pulls(scl->number, false, true);

    common_hal_mcu_delay_us(10);

    gpio_set_pulls(sda->number, false, false);
    gpio_set_pulls(scl->number, false, false);

    // We must pull up within 3us to achieve 400khz.
    common_hal_mcu_delay_us(3);

    if (!gpio_get(sda->number) || !gpio_get(scl->number)) {
        reset_pin_number(sda->number);
        reset_pin_number(scl->number);
        mp_raise_RuntimeError(MP_ERROR_TEXT("No pull up found on SDA or SCL; check your wiring"));
    }
    #endif

    // Create a bitbangio.I2C object to do 0 byte writes.
    //
    // These are used to non-invasively detect I2C devices by sending
    // the address and confirming an ACK.
    // They are not supported by the RP2040 hardware.
    //
    // Must be done before setting up the I2C pins, since they will be
    // set up as GPIO by the bitbangio.I2C object.
    //
    // Sets pins to open drain, high, and input.
    //
    // Do not use the default supplied clock stretching timeout here.
    // It is too short for some devices. Use the busio timeout instead.
    shared_module_bitbangio_i2c_construct(&self->bitbangio_i2c, scl, sda,
        frequency, BUS_TIMEOUT_US);

    self->baudrate = i2c_init(self->peripheral, frequency);

    self->scl_pin = scl->number;
    self->sda_pin = sda->number;
    claim_pin(scl);
    claim_pin(sda);

    gpio_set_function(self->scl_pin, GPIO_FUNC_I2C);
    gpio_set_function(self->sda_pin, GPIO_FUNC_I2C);
}

bool common_hal_busio_i2c_deinited(busio_i2c_obj_t *self) {
    return self->sda_pin == NO_PIN;
}

void common_hal_busio_i2c_deinit(busio_i2c_obj_t *self) {
    if (common_hal_busio_i2c_deinited(self)) {
        return;
    }
    never_reset_i2c[i2c_hw_index(self->peripheral)] = false;

    i2c_deinit(self->peripheral);

    reset_pin_number(self->sda_pin);
    reset_pin_number(self->scl_pin);
    self->sda_pin = NO_PIN;
    self->scl_pin = NO_PIN;
}

bool common_hal_busio_i2c_probe(busio_i2c_obj_t *self, uint8_t addr) {
    return common_hal_busio_i2c_write(self, addr, NULL, 0) == 0;
}

bool common_hal_busio_i2c_try_lock(busio_i2c_obj_t *self) {
    bool grabbed_lock = false;
    if (!self->has_lock) {
        grabbed_lock = true;
        self->has_lock = true;
    }
    return grabbed_lock;
}

bool common_hal_busio_i2c_has_lock(busio_i2c_obj_t *self) {
    return self->has_lock;
}

void common_hal_busio_i2c_unlock(busio_i2c_obj_t *self) {
    self->has_lock = false;
}

static uint8_t _common_hal_busio_i2c_write(busio_i2c_obj_t *self, uint16_t addr,
    const uint8_t *data, size_t len, bool transmit_stop_bit) {
    if (len == 0) {
        // The RP2040 I2C peripheral will not perform 0 byte writes.
        // So use bitbangio.I2C to do the write.

        gpio_set_function(self->scl_pin, GPIO_FUNC_SIO);
        gpio_set_function(self->sda_pin, GPIO_FUNC_SIO);
        gpio_set_dir(self->scl_pin, GPIO_IN);
        gpio_set_dir(self->sda_pin, GPIO_IN);
        gpio_put(self->scl_pin, false);
        gpio_put(self->sda_pin, false);

        uint8_t status = shared_module_bitbangio_i2c_write(&self->bitbangio_i2c,
            addr, data, len, transmit_stop_bit);

        // The pins must be set back to GPIO_FUNC_I2C in the order given here,
        // SCL first, otherwise reads will hang.
        gpio_set_function(self->scl_pin, GPIO_FUNC_I2C);
        gpio_set_function(self->sda_pin, GPIO_FUNC_I2C);

        return status;
    }

    size_t result = i2c_write_timeout_us(self->peripheral, addr, data, len, !transmit_stop_bit, BUS_TIMEOUT_US);
    if (result == len) {
        return 0;
    }
    switch (result) {
        case PICO_ERROR_GENERIC:
            return MP_ENODEV;
        case PICO_ERROR_TIMEOUT:
            return MP_ETIMEDOUT;
        default:
            return MP_EIO;
    }
}

uint8_t common_hal_busio_i2c_write(busio_i2c_obj_t *self, uint16_t addr,
    const uint8_t *data, size_t len) {
    return _common_hal_busio_i2c_write(self, addr, data, len, true);
}

uint8_t common_hal_busio_i2c_read(busio_i2c_obj_t *self, uint16_t addr,
    uint8_t *data, size_t len) {
    size_t result = i2c_read_timeout_us(self->peripheral, addr, data, len, false, BUS_TIMEOUT_US);
    if (result == len) {
        return 0;
    }
    switch (result) {
        case PICO_ERROR_GENERIC:
            return MP_ENODEV;
        case PICO_ERROR_TIMEOUT:
            return MP_ETIMEDOUT;
        default:
            return MP_EIO;
    }
}

uint8_t common_hal_busio_i2c_write_read(busio_i2c_obj_t *self, uint16_t addr,
    uint8_t *out_data, size_t out_len, uint8_t *in_data, size_t in_len) {
    uint8_t result = _common_hal_busio_i2c_write(self, addr, out_data, out_len, false);
    if (result != 0) {
        return result;
    }

    return common_hal_busio_i2c_read(self, addr, in_data, in_len);
}

void common_hal_busio_i2c_never_reset(busio_i2c_obj_t *self) {
    never_reset_i2c[i2c_hw_index(self->peripheral)] = true;

    never_reset_pin_number(self->scl_pin);
    never_reset_pin_number(self->sda_pin);
}
