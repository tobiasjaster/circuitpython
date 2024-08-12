// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Dan Halbert for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <string.h>
#include <stdio.h>

#include "ble_drv.h"
#include "ble_gatts.h"
#include "nrf_nvic.h"

#include "shared/runtime/interrupt_char.h"
#include "py/runtime.h"
#include "py/stream.h"

#include "shared-bindings/_bleio/__init__.h"
#include "shared-bindings/_bleio/Connection.h"
#include "supervisor/shared/tick.h"
#include "common-hal/_bleio/CharacteristicBuffer.h"
#include "shared-bindings/_bleio/CharacteristicBuffer.h"

// Push all the data onto the ring buffer. When the buffer is full, new bytes will be dropped.
static void write_to_ringbuf(bleio_characteristic_buffer_obj_t *self, uint8_t *data, uint16_t len) {
    uint8_t is_nested_critical_region;
    sd_nvic_critical_region_enter(&is_nested_critical_region);
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
    sd_nvic_critical_region_exit(is_nested_critical_region);
}

static bool characteristic_buffer_on_ble_evt(ble_evt_t *ble_evt, void *param) {
    bleio_characteristic_buffer_obj_t *self = (bleio_characteristic_buffer_obj_t *)param;
    switch (ble_evt->header.evt_id) {
        case BLE_GATTS_EVT_WRITE: {
            // A client wrote to this server characteristic.

            ble_gatts_evt_write_t *evt_write = &ble_evt->evt.gatts_evt.params.write;
            // Event handle must match the handle for my characteristic.
            if (evt_write->handle == self->characteristic->handle) {
                write_to_ringbuf(self, evt_write->data, evt_write->len);
            }
            break;
        }

        case BLE_GATTC_EVT_HVX: {
            // A remote service wrote to this characteristic.

            ble_gattc_evt_hvx_t *evt_hvx = &ble_evt->evt.gattc_evt.params.hvx;
            // Must be a notification, and event handle must match the handle for my characteristic.
            if (evt_hvx->type == BLE_GATT_HVX_NOTIFICATION &&
                evt_hvx->handle == self->characteristic->handle) {
                write_to_ringbuf(self, evt_hvx->data, evt_hvx->len);
            }
            break;
        }
        default:
            return false;
            break;
    }
    return true;
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

    if (static_handler_entry != NULL) {
        ble_drv_add_event_handler_entry((ble_drv_evt_handler_entry_t *)static_handler_entry, characteristic_buffer_on_ble_evt, self);
    } else {
        ble_drv_add_event_handler(characteristic_buffer_on_ble_evt, self);
    }
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

    // Copy received data. Lock out write interrupt handler while copying.
    uint8_t is_nested_critical_region;
    sd_nvic_critical_region_enter(&is_nested_critical_region);

    uint32_t num_bytes_read = ringbuf_get_n(&self->ringbuf, data, len);

    // Writes now OK.
    sd_nvic_critical_region_exit(is_nested_critical_region);

    return num_bytes_read;
}

uint32_t common_hal_bleio_characteristic_buffer_rx_characters_available(bleio_characteristic_buffer_obj_t *self) {
    uint8_t is_nested_critical_region;
    sd_nvic_critical_region_enter(&is_nested_critical_region);
    uint16_t count = ringbuf_num_filled(&self->ringbuf);
    sd_nvic_critical_region_exit(is_nested_critical_region);
    return count;
}

void common_hal_bleio_characteristic_buffer_clear_rx_buffer(bleio_characteristic_buffer_obj_t *self) {
    // prevent conflict with uart irq
    uint8_t is_nested_critical_region;
    sd_nvic_critical_region_enter(&is_nested_critical_region);
    ringbuf_clear(&self->ringbuf);
    sd_nvic_critical_region_exit(is_nested_critical_region);
}

bool common_hal_bleio_characteristic_buffer_deinited(bleio_characteristic_buffer_obj_t *self) {
    return self->characteristic == NULL;
}

void common_hal_bleio_characteristic_buffer_deinit(bleio_characteristic_buffer_obj_t *self) {
    if (!common_hal_bleio_characteristic_buffer_deinited(self)) {
        ble_drv_remove_event_handler(characteristic_buffer_on_ble_evt, self);
        self->characteristic = NULL;
        ringbuf_deinit(&self->ringbuf);
    }
}

bool common_hal_bleio_characteristic_buffer_connected(bleio_characteristic_buffer_obj_t *self) {
    return self->characteristic != NULL &&
           self->characteristic->service != NULL &&
           (!self->characteristic->service->is_remote ||
               (self->characteristic->service->connection != MP_OBJ_NULL &&
                   common_hal_bleio_connection_get_connected(self->characteristic->service->connection)));
}
