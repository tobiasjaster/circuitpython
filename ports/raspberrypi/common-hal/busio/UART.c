// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 microDev
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/busio/UART.h"

#include "py/stream.h"
#include "py/mperrno.h"
#include "py/runtime.h"
#include "supervisor/shared/tick.h"
#include "shared/runtime/interrupt_char.h"
#include "common-hal/microcontroller/Pin.h"
#include "shared-bindings/microcontroller/Pin.h"

#include "src/rp2_common/hardware_irq/include/hardware/irq.h"
#include "src/rp2_common/hardware_gpio/include/hardware/gpio.h"

#define NO_PIN 0xff

#define UART_INST(uart) (((uart) ? uart1 : uart0))

typedef enum {
    STATUS_FREE = 0,
    STATUS_BUSY,
    STATUS_NEVER_RESET
} uart_status_t;

static uart_status_t uart_status[NUM_UARTS];

void reset_uart(void) {
    for (uint8_t num = 0; num < NUM_UARTS; num++) {
        if (uart_status[num] == STATUS_BUSY) {
            uart_status[num] = STATUS_FREE;
            uart_deinit(UART_INST(num));
        }
    }
}

void never_reset_uart(uint8_t num) {
    uart_status[num] = STATUS_NEVER_RESET;
}

static void pin_check(const uint8_t uart, const mcu_pin_obj_t *pin, const uint8_t pin_type) {
    if (pin == NULL) {
        return;
    }
    if (!(((pin->number % 4) == pin_type) && ((((pin->number + 4) / 8) % NUM_UARTS) == uart))) {
        raise_ValueError_invalid_pins();
    }
}

static uint8_t pin_init(const uint8_t uart, const mcu_pin_obj_t *pin, const uint8_t pin_type) {
    if (pin == NULL) {
        return NO_PIN;
    }
    claim_pin(pin);
    gpio_set_function(pin->number, GPIO_FUNC_UART);
    return pin->number;
}

static busio_uart_obj_t *active_uarts[NUM_UARTS];

static void _copy_into_ringbuf(ringbuf_t *r, uart_inst_t *uart) {
    while (uart_is_readable(uart) && ringbuf_num_empty(r) > 0) {
        ringbuf_put(r, (uint8_t)uart_get_hw(uart)->dr);
    }
}

static void shared_callback(busio_uart_obj_t *self) {
    _copy_into_ringbuf(&self->ringbuf, self->uart);
    // We always clear the interrupt so it doesn't continue to fire because we
    // may not have read everything available.
    uart_get_hw(self->uart)->icr = UART_UARTICR_RXIC_BITS | UART_UARTICR_RTIC_BITS;
}

static void uart0_callback(void) {
    shared_callback(active_uarts[0]);
}

static void uart1_callback(void) {
    shared_callback(active_uarts[1]);
}

void common_hal_busio_uart_construct(busio_uart_obj_t *self,
    const mcu_pin_obj_t *tx, const mcu_pin_obj_t *rx,
    const mcu_pin_obj_t *rts, const mcu_pin_obj_t *cts,
    const mcu_pin_obj_t *rs485_dir, bool rs485_invert,
    uint32_t baudrate, uint8_t bits, busio_uart_parity_t parity, uint8_t stop,
    mp_float_t timeout, uint16_t receiver_buffer_size, byte *receiver_buffer,
    bool sigint_enabled) {

    mp_arg_validate_int_max(bits, 8, MP_QSTR_bits);
    mp_arg_validate_int_min(receiver_buffer_size, 1, MP_QSTR_receiver_buffer_size);

    uint8_t uart_id = ((((tx != NULL) ? tx->number : rx->number) + 4) / 8) % NUM_UARTS;

    pin_check(uart_id, tx, 0);
    pin_check(uart_id, rx, 1);
    pin_check(uart_id, cts, 2);
    pin_check(uart_id, rts, 3);

    if (uart_status[uart_id] != STATUS_FREE) {
        mp_raise_ValueError(MP_ERROR_TEXT("UART peripheral in use"));
    }

    self->tx_pin = pin_init(uart_id, tx, 0);
    self->rx_pin = pin_init(uart_id, rx, 1);
    self->cts_pin = pin_init(uart_id, cts, 2);
    self->rts_pin = pin_init(uart_id, rts, 3);

    if (rs485_dir != NULL) {
        uint8_t pin = rs485_dir->number;
        self->rs485_dir_pin = pin;
        self->rs485_invert = rs485_invert;

        gpio_init(pin);

        claim_pin(rs485_dir);

        gpio_disable_pulls(pin);

        // Turn on "strong" pin driving (more current available).
        hw_write_masked(&pads_bank0_hw->io[pin],
            PADS_BANK0_GPIO0_DRIVE_VALUE_12MA << PADS_BANK0_GPIO0_DRIVE_LSB,
                PADS_BANK0_GPIO0_DRIVE_BITS);

        gpio_put(self->rs485_dir_pin, rs485_invert);
        gpio_set_dir(self->rs485_dir_pin, GPIO_OUT);
    } else {
        self->rs485_dir_pin = NO_PIN;
    }

    uart_status[uart_id] = STATUS_BUSY;


    self->uart = UART_INST(uart_id);
    self->uart_id = uart_id;
    self->baudrate = baudrate;
    self->timeout_ms = timeout * 1000;

    uart_init(self->uart, self->baudrate);
    uart_set_fifo_enabled(self->uart, true);
    uart_set_format(self->uart, bits, stop, parity);
    uart_set_hw_flow(self->uart, (cts != NULL), (rts != NULL));

    if (rx != NULL) {
        // Use the provided buffer when given.
        if (receiver_buffer != NULL) {
            ringbuf_init(&self->ringbuf, receiver_buffer, receiver_buffer_size);
        } else {
            if (!ringbuf_alloc(&self->ringbuf, receiver_buffer_size)) {
                uart_deinit(self->uart);
                m_malloc_fail(receiver_buffer_size);
            }
        }
    }

    active_uarts[uart_id] = self;
    if (uart_id == 1) {
        self->uart_irq_id = UART1_IRQ;
        irq_set_exclusive_handler(self->uart_irq_id, uart1_callback);
    } else {
        self->uart_irq_id = UART0_IRQ;
        irq_set_exclusive_handler(self->uart_irq_id, uart0_callback);
    }
    irq_set_enabled(self->uart_irq_id, true);
    uart_set_irq_enables(self->uart, true /* rx has data */, false /* tx needs data */);
}

bool common_hal_busio_uart_deinited(busio_uart_obj_t *self) {
    return self->tx_pin == NO_PIN && self->rx_pin == NO_PIN;
}

void common_hal_busio_uart_deinit(busio_uart_obj_t *self) {
    if (common_hal_busio_uart_deinited(self)) {
        return;
    }
    uart_deinit(self->uart);
    ringbuf_deinit(&self->ringbuf);
    active_uarts[self->uart_id] = NULL;
    uart_status[self->uart_id] = STATUS_FREE;
    reset_pin_number(self->tx_pin);
    reset_pin_number(self->rx_pin);
    reset_pin_number(self->cts_pin);
    reset_pin_number(self->rts_pin);
    reset_pin_number(self->rs485_dir_pin);
    self->tx_pin = NO_PIN;
    self->rx_pin = NO_PIN;
    self->cts_pin = NO_PIN;
    self->rts_pin = NO_PIN;
    self->rs485_dir_pin = NO_PIN;
}

// Write characters.
size_t common_hal_busio_uart_write(busio_uart_obj_t *self, const uint8_t *data, size_t len, int *errcode) {
    if (self->tx_pin == NO_PIN) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("No %q pin"), MP_QSTR_tx);
    }

    if (self->rs485_dir_pin != NO_PIN) {
        uart_tx_wait_blocking(self->uart);
        gpio_put(self->rs485_dir_pin, !self->rs485_invert);
    }

    size_t left_to_write = len;
    while (left_to_write > 0) {
        while (uart_is_writable(self->uart) && left_to_write > 0) {
            // Write and advance.
            uart_get_hw(self->uart)->dr = *data++;
            // Decrease how many chars left to write.
            left_to_write--;
        }
        RUN_BACKGROUND_TASKS;
    }
    if (self->rs485_dir_pin != NO_PIN) {
        uart_tx_wait_blocking(self->uart);
        gpio_put(self->rs485_dir_pin, self->rs485_invert);
    }
    return len;
}

// Read characters.
size_t common_hal_busio_uart_read(busio_uart_obj_t *self, uint8_t *data, size_t len, int *errcode) {
    if (self->rx_pin == NO_PIN) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("No %q pin"), MP_QSTR_rx);
    }

    if (len == 0) {
        // Nothing to read.
        return 0;
    }

    // Prevent conflict with uart irq.
    irq_set_enabled(self->uart_irq_id, false);

    // Copy as much received data as available, up to len bytes.
    size_t total_read = ringbuf_get_n(&self->ringbuf, data, len);

    // Check if we still need to read more data.
    if (len > total_read) {
        len -= total_read;
        uint64_t start_ticks = supervisor_ticks_ms64();
        // Busy-wait until timeout or until we've read enough chars.
        while (len > 0 && (supervisor_ticks_ms64() - start_ticks < self->timeout_ms)) {
            if (uart_is_readable(self->uart)) {
                // Read and advance.
                data[total_read] = uart_get_hw(self->uart)->dr;

                // Adjust the counters.
                len--;
                total_read++;

                // Reset the timeout on every character read.
                start_ticks = supervisor_ticks_ms64();
            }
            RUN_BACKGROUND_TASKS;
            // Allow user to break out of a timeout with a KeyboardInterrupt.
            if (mp_hal_is_interrupted()) {
                break;
            }
        }
    }

    // Now that we've emptied the ringbuf some, fill it up with anything in the
    // FIFO. This ensures that we'll empty the FIFO as much as possible and
    // reset the interrupt when we catch up.
    _copy_into_ringbuf(&self->ringbuf, self->uart);

    // Re-enable irq.
    irq_set_enabled(self->uart_irq_id, true);

    if (total_read == 0) {
        *errcode = EAGAIN;
        return MP_STREAM_ERROR;
    }

    return total_read;
}

uint32_t common_hal_busio_uart_get_baudrate(busio_uart_obj_t *self) {
    return self->baudrate;
}

void common_hal_busio_uart_set_baudrate(busio_uart_obj_t *self, uint32_t baudrate) {
    self->baudrate = baudrate;
    uart_set_baudrate(self->uart, baudrate);
}

mp_float_t common_hal_busio_uart_get_timeout(busio_uart_obj_t *self) {
    return (mp_float_t)(self->timeout_ms / 1000.0f);
}

void common_hal_busio_uart_set_timeout(busio_uart_obj_t *self, mp_float_t timeout) {
    self->timeout_ms = timeout * 1000;
}

uint32_t common_hal_busio_uart_rx_characters_available(busio_uart_obj_t *self) {
    // Prevent conflict with uart irq.
    irq_set_enabled(self->uart_irq_id, false);
    // The UART only interrupts after a threshold so make sure to copy anything
    // out of its FIFO before measuring how many bytes we've received.
    _copy_into_ringbuf(&self->ringbuf, self->uart);
    irq_set_enabled(self->uart_irq_id, true);
    return ringbuf_num_filled(&self->ringbuf);
}

void common_hal_busio_uart_clear_rx_buffer(busio_uart_obj_t *self) {
    // Prevent conflict with uart irq.
    irq_set_enabled(self->uart_irq_id, false);
    ringbuf_clear(&self->ringbuf);

    // Throw away the FIFO contents too.
    while (uart_is_readable(self->uart)) {
        (void)uart_get_hw(self->uart)->dr;
    }
    irq_set_enabled(self->uart_irq_id, true);
}

bool common_hal_busio_uart_ready_to_tx(busio_uart_obj_t *self) {
    if (self->tx_pin == NO_PIN) {
        return false;
    }
    return uart_is_writable(self->uart);
}

static void pin_never_reset(uint8_t pin) {
    if (pin != NO_PIN) {
        never_reset_pin_number(pin);
    }
}

void common_hal_busio_uart_never_reset(busio_uart_obj_t *self) {
    never_reset_uart(self->uart_id);
    pin_never_reset(self->tx_pin);
    pin_never_reset(self->rx_pin);
    pin_never_reset(self->cts_pin);
    pin_never_reset(self->rs485_dir_pin);
    pin_never_reset(self->rts_pin);
}
