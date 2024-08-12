// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2016 Damien P. George
//
// SPDX-License-Identifier: MIT

#if CIRCUITPY_BUSIO_UART
#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/busio/UART.h"

#include "mpconfigport.h"
#include "shared/runtime/interrupt_char.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "supervisor/shared/tick.h"

#include "hpl_sercom_config.h"
#include "peripheral_clk_config.h"

#include "hal/include/hal_gpio.h"
#include "hal/include/hal_usart_async.h"
#include "hal/include/hpl_usart_async.h"

#include "samd/sercom.h"

#include "common-hal/busio/__init__.h"

#define UART_DEBUG(...) (void)0
// #define UART_DEBUG(...) mp_printf(&mp_plat_print __VA_OPT__(,) __VA_ARGS__)

// Do-nothing callback needed so that usart_async code will enable rx interrupts.
// See comment below re usart_async_register_callback()
static void usart_async_rxc_callback(const struct usart_async_descriptor *const descr) {
    // Nothing needs to be done by us.
}

// shared-bindings validates that the tx and rx are not both missing,
// and that the pins are distinct.
void common_hal_busio_uart_construct(busio_uart_obj_t *self,
    const mcu_pin_obj_t *tx, const mcu_pin_obj_t *rx,
    const mcu_pin_obj_t *rts, const mcu_pin_obj_t *cts,
    const mcu_pin_obj_t *rs485_dir, bool rs485_invert,
    uint32_t baudrate, uint8_t bits, busio_uart_parity_t parity, uint8_t stop,
    mp_float_t timeout, uint16_t receiver_buffer_size, byte *receiver_buffer,
    bool sigint_enabled) {

    Sercom *sercom = NULL;
    uint8_t sercom_index = 255; // Unset index
    uint32_t rx_pinmux = 0;
    uint8_t rx_pad = 255; // Unset pad
    uint32_t tx_pinmux = 0;
    uint8_t tx_pad = 255; // Unset pad
    uint32_t rts_pinmux = 0;
    uint32_t cts_pinmux = 0;

    // Set state so the object is deinited to start.
    self->rx_pin = NO_PIN;
    self->tx_pin = NO_PIN;
    self->rts_pin = NO_PIN;
    self->cts_pin = NO_PIN;

    if ((rs485_dir != NULL) || (rs485_invert)) {
        mp_raise_NotImplementedError(MP_ERROR_TEXT("RS485"));
    }

    mp_arg_validate_int_max(bits, 8, MP_QSTR_bits);

    bool have_tx = tx != NULL;
    bool have_rx = rx != NULL;
    bool have_rts = rts != NULL;
    bool have_cts = cts != NULL;

    if (have_rx && receiver_buffer_size > 0 && (receiver_buffer_size & (receiver_buffer_size - 1)) != 0) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("%q must be power of 2"), MP_QSTR_receiver_buffer_size);
    }

    self->baudrate = baudrate;
    self->character_bits = bits;
    self->timeout_ms = timeout * 1000;

    // This assignment is only here because the usart_async routines take a *const argument.
    struct usart_async_descriptor *const usart_desc_p = (struct usart_async_descriptor *const)&self->usart_desc;

    // Allowed pads for USART. See the SAMD21 and SAMx5x datasheets.
    // TXPO:
    // (both)     0x0: TX pad 0; no RTS/CTS
    // (SAMD21)   0x1: TX pad 2; no RTS/CTS
    // (SAMx5x)   0x1: reserved
    // (both)     0x2: TX pad 0; RTS: pad 2, CTS: pad 3
    // (SAMD21)   0x3: reserved
    // (SAMx5x)   0x3: TX pad 0; RTS: pad 2; no CTS
    // RXPO:
    // 0x0: RX pad 0
    // 0x1: RX pad 1
    // 0x2: RX pad 2
    // 0x3: RX pad 3

    for (int i = 0; i < NUM_SERCOMS_PER_PIN; i++) {
        Sercom *potential_sercom = NULL;
        if (have_tx) {
            sercom_index = tx->sercom[i].index;
            if (sercom_index >= SERCOM_INST_NUM) {
                continue;
            }
            potential_sercom = sercom_insts[sercom_index];

            // SAMD21 and SAMx5x have different requirements.

            #ifdef SAMD21
            if (potential_sercom->USART.CTRLA.bit.ENABLE != 0) {
                // In use.
                continue;
            }
            if (tx->sercom[i].pad != 0 &&
                tx->sercom[i].pad != 2) {
                // TX must be on pad 0 or 2.
                continue;
            }
            if (have_rts) {
                if (rts->sercom[i].pad != 2 ||
                    tx->sercom[i].pad == 2) {
                    // RTS pin must be on pad 2, so if TX is also on pad 2, not possible
                    continue;
                }
            }
            if (have_cts) {
                if (cts->sercom[i].pad != 3 ||
                    (have_rx && rx->sercom[i].pad == 3)) {
                    // CTS pin must be on pad 3, so if RX is also on pad 3, not possible
                    continue;
                }
            }
            #endif

            #ifdef SAM_D5X_E5X
            if (potential_sercom->USART.CTRLA.bit.ENABLE != 0) {
                // In use.
                continue;
            }
            if (tx->sercom[i].pad != 0) {
                // TX must be on pad 0
                continue;
            }

            if (have_rts && rts->sercom[i].pad != 2) {
                // RTS pin must be on pad 2
                continue;
            }
            if (have_cts) {
                if (cts->sercom[i].pad != 3 ||
                    (have_rx && rx->sercom[i].pad == 3)) {
                    // CTS pin must be on pad 3, so if RX is also on pad 3, not possible
                    continue;
                }
            }
            #endif

            tx_pinmux = PINMUX(tx->number, (i == 0) ? MUX_C : MUX_D);
            tx_pad = tx->sercom[i].pad;
            if (have_rts) {
                rts_pinmux = PINMUX(rts->number, (i == 0) ? MUX_C : MUX_D);
            }
            if (!have_rx) {
                // TX only, so don't need to look further.
                sercom = potential_sercom;
                break;
            }
        }

        // Have TX, now look for RX match. We know have_rx is true at this point.
        for (int j = 0; j < NUM_SERCOMS_PER_PIN; j++) {
            if (((!have_tx && rx->sercom[j].index < SERCOM_INST_NUM &&
                  sercom_insts[rx->sercom[j].index]->USART.CTRLA.bit.ENABLE == 0) ||
                 sercom_index == rx->sercom[j].index) &&
                rx->sercom[j].pad != tx_pad) {
                rx_pinmux = PINMUX(rx->number, (j == 0) ? MUX_C : MUX_D);
                rx_pad = rx->sercom[j].pad;
                if (have_cts) {
                    cts_pinmux = PINMUX(cts->number, (j == 0) ? MUX_C : MUX_D);
                }
                sercom = sercom_insts[rx->sercom[j].index];
                sercom_index = rx->sercom[j].index;
                break;
            }
        }
        if (sercom != NULL) {
            break;
        }
    }
    if (sercom == NULL) {
        raise_ValueError_invalid_pins();
    }
    // Set up clocks on SERCOM.
    samd_peripherals_sercom_clock_init(sercom, sercom_index);

    if (have_rx && receiver_buffer_size > 0) {
        self->buffer_length = receiver_buffer_size;
        if (NULL != receiver_buffer) {
            self->buffer = receiver_buffer;
        } else {
            self->buffer = (uint8_t *)gc_alloc(self->buffer_length * sizeof(uint8_t), false);
            if (self->buffer == NULL) {
                common_hal_busio_uart_deinit(self);
                m_malloc_fail(self->buffer_length * sizeof(uint8_t));
            }
        }
    } else {
        self->buffer_length = 0;
        self->buffer = NULL;
    }

    if (usart_async_init(usart_desc_p, sercom, self->buffer, self->buffer_length, NULL) != ERR_NONE) {
        mp_raise_RuntimeError(NULL);
    }

    // usart_async_init() sets a number of defaults based on a prototypical SERCOM
    // which don't necessarily match what we need. After calling it, set the values
    // specific to this instantiation of UART.

    // See the TXPO/RXPO table above for how RXPO and TXPO are chosen below.

    // rxpo maps directly to rx_pad.
    // Set to 0x0 if no RX, but it doesn't matter because RX will not be enabled.
    const uint8_t rxpo = have_rx ? rx_pad : 0x0;

    #ifdef SAMD21
    // SAMD21 has only one txpo value when using either CTS or RTS or both.
    // TX is on pad 0 or 2, or there is no TX.
    // 0x0 for pad 0, 0x1 for pad 2.
    uint8_t txpo;
    if (tx_pad == 2) {
        txpo = 0x1;
    } else {
        txpo = (have_cts || have_rts) ? 0x2 : 0x0;
    }
    #endif

    #ifdef SAM_D5X_E5X
    // SAMx5x has two different possibilities, per the chart above.
    // We already know TX is on pad 0, or there is no TX.

    // Without RTS or CTS, txpo can be 0x0.
    // It's not clear if 0x2 would cover all our cases, but this is known to be safe.
    uint8_t txpo = (have_rts || have_cts) ? 0x2: 0x0;
    #endif

    // Doing a group mask and set of the registers saves 60 bytes over setting the bitfields individually.

    sercom->USART.CTRLA.reg &= ~(SERCOM_USART_CTRLA_TXPO_Msk |
        SERCOM_USART_CTRLA_RXPO_Msk |
        SERCOM_USART_CTRLA_FORM_Msk);
    // See chart above for TXPO values and RXPO values.
    sercom->USART.CTRLA.reg |= SERCOM_USART_CTRLA_TXPO(txpo) |
        SERCOM_USART_CTRLA_RXPO(rxpo) |
        (parity == BUSIO_UART_PARITY_NONE ? 0 : SERCOM_USART_CTRLA_FORM(1));

    // Enable tx and/or rx based on whether the pins were specified.
    // CHSIZE is 0 for 8 bits, 5, 6, 7 for 5, 6, 7 bits. 1 for 9 bits, but we don't support that.
    sercom->USART.CTRLB.reg &= ~(SERCOM_USART_CTRLB_TXEN |
        SERCOM_USART_CTRLB_RXEN |
        SERCOM_USART_CTRLB_PMODE |
        SERCOM_USART_CTRLB_SBMODE |
        SERCOM_USART_CTRLB_CHSIZE_Msk);
    sercom->USART.CTRLB.reg |= (have_tx ? SERCOM_USART_CTRLB_TXEN : 0) |
        (have_rx ? SERCOM_USART_CTRLB_RXEN : 0) |
        (parity == BUSIO_UART_PARITY_ODD ? SERCOM_USART_CTRLB_PMODE : 0) |
        (stop > 1 ? SERCOM_USART_CTRLB_SBMODE : 0) |
        SERCOM_USART_CTRLB_CHSIZE(bits % 8);

    // Set baud rate
    common_hal_busio_uart_set_baudrate(self, baudrate);

    // Turn on rx interrupt handling. The UART async driver has its own set of internal callbacks,
    // which are set up by uart_async_init(). These in turn can call user-specified callbacks.
    // In fact, the actual interrupts are not enabled unless we set up a user-specified callback.
    // This is confusing. It's explained in the Atmel START User Guide -> Implementation Description ->
    // Different read function behavior in some asynchronous drivers. As of this writing:
    // http://start.atmel.com/static/help/index.html?GUID-79201A5A-226F-4FBB-B0B8-AB0BE0554836
    // Look at the ASFv4 code example for async USART.
    usart_async_register_callback(usart_desc_p, USART_ASYNC_RXC_CB, usart_async_rxc_callback);


    if (have_tx) {
        gpio_set_pin_direction(tx->number, GPIO_DIRECTION_OUT);
        gpio_set_pin_pull_mode(tx->number, GPIO_PULL_OFF);
        gpio_set_pin_function(tx->number, tx_pinmux);
        self->tx_pin = tx->number;
        claim_pin(tx);
    } else {
        self->tx_pin = NO_PIN;
    }

    if (have_rx) {
        gpio_set_pin_direction(rx->number, GPIO_DIRECTION_IN);
        gpio_set_pin_pull_mode(rx->number, GPIO_PULL_OFF);
        gpio_set_pin_function(rx->number, rx_pinmux);
        self->rx_pin = rx->number;
        claim_pin(rx);
    } else {
        self->rx_pin = NO_PIN;
    }

    if (have_rts) {
        gpio_set_pin_direction(rts->number, GPIO_DIRECTION_OUT);
        gpio_set_pin_pull_mode(rts->number, GPIO_PULL_OFF);
        gpio_set_pin_function(rts->number, rts_pinmux);
        self->rts_pin = rts->number;
        claim_pin(rts);
    } else {
        self->rts_pin = NO_PIN;
    }

    if (have_cts) {
        gpio_set_pin_direction(cts->number, GPIO_DIRECTION_IN);
        gpio_set_pin_pull_mode(cts->number, GPIO_PULL_OFF);
        gpio_set_pin_function(cts->number, cts_pinmux);
        self->cts_pin = cts->number;
        claim_pin(cts);
    } else {
        self->cts_pin = NO_PIN;
    }

    usart_async_enable(usart_desc_p);
}

void common_hal_busio_uart_never_reset(busio_uart_obj_t *self) {
    for (size_t i = 0; i < MP_ARRAY_SIZE(sercom_insts); i++) {
        const Sercom *sercom = sercom_insts[i];
        Sercom *hw = (Sercom *)(self->usart_desc.device.hw);

        // Reserve pins for active UART only
        if (sercom == hw) {
            never_reset_sercom(hw);
            never_reset_pin_number(self->rx_pin);
            never_reset_pin_number(self->tx_pin);
            never_reset_pin_number(self->rts_pin);
            never_reset_pin_number(self->cts_pin);
        }
    }
    return;
}

bool common_hal_busio_uart_deinited(busio_uart_obj_t *self) {
    return self->rx_pin == NO_PIN && self->tx_pin == NO_PIN;
}

void common_hal_busio_uart_deinit(busio_uart_obj_t *self) {
    if (common_hal_busio_uart_deinited(self)) {
        return;
    }
    // This assignment is only here because the usart_async routines take a *const argument.
    struct usart_async_descriptor *const usart_desc_p = (struct usart_async_descriptor *const)&self->usart_desc;
    usart_async_disable(usart_desc_p);
    usart_async_deinit(usart_desc_p);
    reset_pin_number(self->rx_pin);
    reset_pin_number(self->tx_pin);
    reset_pin_number(self->rts_pin);
    reset_pin_number(self->cts_pin);
    self->rx_pin = NO_PIN;
    self->tx_pin = NO_PIN;
    self->rts_pin = NO_PIN;
    self->cts_pin = NO_PIN;
}

// Read characters.
size_t common_hal_busio_uart_read(busio_uart_obj_t *self, uint8_t *data, size_t len, int *errcode) {
    if (self->rx_pin == NO_PIN) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("No %q pin"), MP_QSTR_rx);
    }

    // This assignment is only here because the usart_async routines take a *const argument.
    struct usart_async_descriptor *const usart_desc_p = (struct usart_async_descriptor *const)&self->usart_desc;

    if (len == 0) {
        // Nothing to read.
        return 0;
    }

    struct io_descriptor *io;
    usart_async_get_io_descriptor(usart_desc_p, &io);

    size_t total_read = 0;
    uint64_t start_ticks = supervisor_ticks_ms64();

    // Busy-wait until timeout or until we've read enough chars.
    while (supervisor_ticks_ms64() - start_ticks <= self->timeout_ms) {
        // Read as many chars as we can right now, up to len.
        size_t num_read = io_read(io, data, len);

        // Advance pointer in data buffer, and decrease how many chars left to read.
        data += num_read;
        len -= num_read;
        total_read += num_read;
        if (len == 0) {
            // Don't need to read any more: data buf is full.
            break;
        }
        if (num_read > 0) {
            // Reset the timeout on every character read.
            start_ticks = supervisor_ticks_ms64();
        }
        RUN_BACKGROUND_TASKS;
        // Allow user to break out of a timeout with a KeyboardInterrupt.
        if (mp_hal_is_interrupted()) {
            break;
        }
        // If we are zero timeout, make sure we don't loop again (in the event
        // we read in under 1ms)
        if (self->timeout_ms == 0) {
            break;
        }
    }

    if (total_read == 0) {
        *errcode = EAGAIN;
        return MP_STREAM_ERROR;
    }

    return total_read;
}

// Write characters.
size_t common_hal_busio_uart_write(busio_uart_obj_t *self, const uint8_t *data, size_t len, int *errcode) {
    if (self->tx_pin == NO_PIN) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("No %q pin"), MP_QSTR_tx);
    }

    // This assignment is only here because the usart_async routines take a *const argument.
    struct usart_async_descriptor *const usart_desc_p = (struct usart_async_descriptor *const)&self->usart_desc;

    struct io_descriptor *io;
    usart_async_get_io_descriptor(usart_desc_p, &io);

    // Start writing characters. This is non-blocking and will
    // return immediately after setting up the write.
    if (io_write(io, data, len) < 0) {
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }

    // Busy-wait until all characters transmitted.
    struct usart_async_status async_status;
    while (true) {
        usart_async_get_status(usart_desc_p, &async_status);
        if (async_status.txcnt >= len) {
            break;
        }
        RUN_BACKGROUND_TASKS;
    }

    return len;
}

uint32_t common_hal_busio_uart_get_baudrate(busio_uart_obj_t *self) {
    return self->baudrate;
}

void common_hal_busio_uart_set_baudrate(busio_uart_obj_t *self, uint32_t baudrate) {
    // This assignment is only here because the usart_async routines take a *const argument.
    struct usart_async_descriptor *const usart_desc_p = (struct usart_async_descriptor *const)&self->usart_desc;
    usart_async_set_baud_rate(usart_desc_p,
        // Samples and ARITHMETIC vs FRACTIONAL must correspond to USART_SAMPR in
        // hpl_sercom_config.h.
        _usart_async_calculate_baud_rate(baudrate,                        // e.g. 9600 baud
            PROTOTYPE_SERCOM_USART_ASYNC_CLOCK_FREQUENCY,
            16,                                                      // samples
            USART_BAUDRATE_ASYNCH_ARITHMETIC,
            0                                                     // fraction - not used for ARITHMETIC
            ));
    self->baudrate = baudrate;
}

mp_float_t common_hal_busio_uart_get_timeout(busio_uart_obj_t *self) {
    return (mp_float_t)(self->timeout_ms / 1000.0f);
}

void common_hal_busio_uart_set_timeout(busio_uart_obj_t *self, mp_float_t timeout) {
    self->timeout_ms = timeout * 1000;
}

uint32_t common_hal_busio_uart_rx_characters_available(busio_uart_obj_t *self) {
    // This assignment is only here because the usart_async routines take a *const argument.
    struct usart_async_descriptor *const usart_desc_p = (struct usart_async_descriptor *const)&self->usart_desc;
    struct usart_async_status async_status;
    usart_async_get_status(usart_desc_p, &async_status);
    return async_status.rxcnt;
}

void common_hal_busio_uart_clear_rx_buffer(busio_uart_obj_t *self) {
    // This assignment is only here because the usart_async routines take a *const argument.
    struct usart_async_descriptor *const usart_desc_p = (struct usart_async_descriptor *const)&self->usart_desc;
    usart_async_flush_rx_buffer(usart_desc_p);

}

// True if there are no characters still to be written.
bool common_hal_busio_uart_ready_to_tx(busio_uart_obj_t *self) {
    if (self->tx_pin == NO_PIN) {
        return false;
    }
    // This assignment is only here because the usart_async routines take a *const argument.
    struct usart_async_descriptor *const usart_desc_p = (struct usart_async_descriptor *const)&self->usart_desc;
    struct usart_async_status async_status;
    usart_async_get_status(usart_desc_p, &async_status);
    return !(async_status.flags & USART_ASYNC_STATUS_BUSY);
}
#endif
