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
    self->initial_value = mp_obj_new_bytes(initial_value_bufinfo->buf, initial_value_bufinfo->len);

    const mp_int_t max_length_max = fixed_length ? BLE_GATTS_FIX_ATTR_LEN_MAX : BLE_GATTS_VAR_ATTR_LEN_MAX;
    if (max_length < 0 || max_length > max_length_max) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("max_length must be 0-%d when fixed_length is %s"),
            max_length_max, fixed_length ? "True" : "False");
    }
    self->max_length = max_length;
    self->fixed_length = fixed_length;
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
        uint16_t conn_handle = bleio_connection_get_conn_handle(self->characteristic->service->connection);
        if (common_hal_bleio_service_get_is_remote(self->characteristic->service)) {
            return common_hal_bleio_gattc_read(self->handle, conn_handle, buf, len);
        } else {
            return common_hal_bleio_gatts_read(self->handle, conn_handle, buf, len);
        }
    }

    return 0;
}

void common_hal_bleio_descriptor_set_value(bleio_descriptor_obj_t *self, mp_buffer_info_t *bufinfo) {
    // Do GATT operations only if this descriptor has been registered.
    if (self->handle != BLE_GATT_HANDLE_INVALID) {
        uint16_t conn_handle = bleio_connection_get_conn_handle(self->characteristic->service->connection);
        if (common_hal_bleio_service_get_is_remote(self->characteristic->service)) {
            // false means WRITE_REQ, not write-no-response
            common_hal_bleio_gattc_write(self->handle, conn_handle, bufinfo, false);
        } else {
            // Validate data length for local descriptors only.
            if (self->fixed_length && bufinfo->len != self->max_length) {
                mp_raise_ValueError(MP_ERROR_TEXT("Value length != required fixed length"));
            }
            if (bufinfo->len > self->max_length) {
                mp_raise_ValueError(MP_ERROR_TEXT("Value length > max_length"));
            }

            common_hal_bleio_gatts_write(self->handle, conn_handle, bufinfo);
        }
    }

}
