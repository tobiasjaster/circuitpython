// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Dan Halbert for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2018 Artur Pacholec
// SPDX-FileCopyrightText: Copyright (c) 2016 Glenn Ruben Bakke
//
// SPDX-License-Identifier: MIT

#include "py/runtime.h"

#include "shared-bindings/_bleio/__init__.h"
#include "shared-bindings/_bleio/Descriptor.h"
#include "shared-bindings/_bleio/Service.h"
#include "shared-bindings/_bleio/UUID.h"

void common_hal_bleio_descriptor_construct(bleio_descriptor_obj_t *self, bleio_characteristic_obj_t *characteristic, bleio_uuid_obj_t *uuid, bleio_attribute_security_mode_t read_perm, bleio_attribute_security_mode_t write_perm, mp_int_t max_length, bool fixed_length, mp_buffer_info_t *initial_value_bufinfo) {
    self->characteristic = characteristic;
    self->uuid = uuid;
    self->handle = BLE_GATT_HANDLE_INVALID;
    self->read_perm = read_perm;
    self->write_perm = write_perm;
    self->value = mp_obj_new_bytes(initial_value_bufinfo->buf, initial_value_bufinfo->len);

    const mp_int_t max_length_max = fixed_length ? BLE_GATTS_FIX_ATTR_LEN_MAX : BLE_GATTS_VAR_ATTR_LEN_MAX;
    if (max_length < 0 || max_length > max_length_max) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("max_length must be 0-%d when fixed_length is %s"),
            max_length_max, fixed_length ? "True" : "False");
    }
    self->max_length = max_length;
    self->fixed_length = fixed_length;

    common_hal_bleio_descriptor_set_value(self, initial_value_bufinfo);
}

bleio_uuid_obj_t *common_hal_bleio_descriptor_get_uuid(bleio_descriptor_obj_t *self) {
    return self->uuid;
}

bleio_characteristic_obj_t *common_hal_bleio_descriptor_get_characteristic(bleio_descriptor_obj_t *self) {
    return self->characteristic;
}

size_t common_hal_bleio_descriptor_get_value(bleio_descriptor_obj_t *self, uint8_t *buf, size_t len) {
    // Do GATT operations only if this descriptor has been registered
    if (self->handle != BLE_GATT_HANDLE_INVALID) {
        if (common_hal_bleio_service_get_is_remote(self->characteristic->service)) {
            // uint16_t conn_handle = bleio_connection_get_conn_handle(self->characteristic->service->connection);
            // FIX have att_read_req fill in a buffer
            // uint8_t rsp[MAX(len, 512)];
            // return att_read_req(conn_handle, self->handle, rsp, len);
            return 0;
        } else {
            mp_buffer_info_t bufinfo;
            if (!mp_get_buffer(self->value, &bufinfo, MP_BUFFER_READ)) {
                return 0;
            }
            const size_t actual_length = MIN(len, bufinfo.len);
            memcpy(buf, bufinfo.buf, actual_length);
            return actual_length;
        }
    }

    return 0;
}

void common_hal_bleio_descriptor_set_value(bleio_descriptor_obj_t *self, mp_buffer_info_t *bufinfo) {
    if (self->fixed_length && bufinfo->len != self->max_length) {
        mp_raise_ValueError(MP_ERROR_TEXT("Value length != required fixed length"));
    }
    if (bufinfo->len > self->max_length) {
        mp_raise_ValueError(MP_ERROR_TEXT("Value length > max_length"));
    }

    self->value = mp_obj_new_bytes(bufinfo->buf, bufinfo->len);

    // Do GATT operations only if this descriptor has been registered.
    if (self->handle != BLE_GATT_HANDLE_INVALID) {
        if (common_hal_bleio_service_get_is_remote(self->characteristic->service)) {
            // FIX
            // uint16_t conn_handle = bleio_connection_get_conn_handle(self->service->connection);
            // att_write_req(conn_handle, self->handle, bufinfo->buf, bufinfo->len, rsp);
        } else {
            // Always write the value locally even if no connections are active.
            if (self->fixed_length && bufinfo->len != self->max_length) {
                return;
            }
            if (bufinfo->len > self->max_length) {
                return;
            }

            self->value = mp_obj_new_bytes(bufinfo->buf, bufinfo->len);
        }
    }
}
