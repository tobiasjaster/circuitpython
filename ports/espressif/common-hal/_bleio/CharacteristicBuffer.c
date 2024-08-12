// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Dan Halbert for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <string.h>
#include <stdio.h>

#include "py/ringbuf.h"
#include "py/runtime.h"
#include "py/stream.h"

#include "shared/runtime/interrupt_char.h"

#include "shared-bindings/_bleio/__init__.h"
#include "shared-bindings/_bleio/Connection.h"
#include "shared-bindings/_bleio/CharacteristicBuffer.h"

#include "supervisor/shared/tick.h"

#include "common-hal/_bleio/ble_events.h"

void bleio_characteristic_buffer_extend(bleio_characteristic_buffer_obj_t *self, const uint8_t *data, size_t len) {
    if (self->watch_for_interrupt_char) {
        for (uint16_t i = 0; i < len; i++) {
            if (data[i] == mp_interrupt_char) {
                mp_sched_keyboard_interrupt();
            } else {
                ringbuf_put(&self->ringbuf, data[i]);
            }
        }
    } else {
        ringbuf_put_n(&self->ringbuf, data, len);
    }
}

void _common_hal_bleio_characteristic_buffer_construct(bleio_characteristic_buffer_obj_t *self,
    bleio_characteristic_obj_t *characteristic,
    mp_float_t timeout,
    uint8_t *buffer, size_t buffer_size,
    void *static_handler_entry,
    bool watch_for_interrupt_char) {
    self->characteristic = characteristic;
    self->timeout_ms = timeout * 1000;
    self->watch_for_interrupt_char = watch_for_interrupt_char;
    ringbuf_init(&self->ringbuf, buffer, buffer_size);
    bleio_characteristic_set_observer(characteristic, self);
}

// Assumes that timeout and buffer_size have been validated before call.
void common_hal_bleio_characteristic_buffer_construct(bleio_characteristic_buffer_obj_t *self,
    bleio_characteristic_obj_t *characteristic,
    mp_float_t timeout,
    size_t buffer_size) {
    uint8_t *buffer = m_malloc(buffer_size);
    _common_hal_bleio_characteristic_buffer_construct(self, characteristic, timeout, buffer, buffer_size, NULL, false);
}

uint32_t common_hal_bleio_characteristic_buffer_read(bleio_characteristic_buffer_obj_t *self, uint8_t *data, size_t len, int *errcode) {
    uint64_t start_ticks = supervisor_ticks_ms64();

    // Wait for all bytes received or timeout
    while ((ringbuf_num_filled(&self->ringbuf) < len) && (supervisor_ticks_ms64() - start_ticks < self->timeout_ms)) {
        RUN_BACKGROUND_TASKS;
        // Allow user to break out of a timeout with a KeyboardInterrupt.
        if (mp_hal_is_interrupted()) {
            return 0;
        }
    }

    uint32_t num_bytes_read = ringbuf_get_n(&self->ringbuf, data, len);
    return num_bytes_read;
}

// NOTE: The nRF port has protection around these operations because the ringbuf
// is filled from an interrupt. On ESP the ringbuf is filled from the BLE host
// task that won't interrupt us.

uint32_t common_hal_bleio_characteristic_buffer_rx_characters_available(bleio_characteristic_buffer_obj_t *self) {
    return ringbuf_num_filled(&self->ringbuf);
}

void common_hal_bleio_characteristic_buffer_clear_rx_buffer(bleio_characteristic_buffer_obj_t *self) {
    ringbuf_clear(&self->ringbuf);
}

bool common_hal_bleio_characteristic_buffer_deinited(bleio_characteristic_buffer_obj_t *self) {
    return self->characteristic == NULL;
}

void common_hal_bleio_characteristic_buffer_deinit(bleio_characteristic_buffer_obj_t *self) {
    if (common_hal_bleio_characteristic_buffer_deinited(self)) {
        return;
    }
    bleio_characteristic_clear_observer(self->characteristic);
    self->characteristic = NULL;
    ringbuf_deinit(&self->ringbuf);
}

bool common_hal_bleio_characteristic_buffer_connected(bleio_characteristic_buffer_obj_t *self) {
    return self->characteristic != NULL &&
           self->characteristic->service != NULL &&
           (!self->characteristic->service->is_remote ||
               (self->characteristic->service->connection != MP_OBJ_NULL &&
                   common_hal_bleio_connection_get_connected(self->characteristic->service->connection)));
}
