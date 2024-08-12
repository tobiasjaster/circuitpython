// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Dan Halbert for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2018 Artur Pacholec
// SPDX-FileCopyrightText: Copyright (c) 2017 Glenn Ruben Bakke
//
// SPDX-License-Identifier: MIT

#include <string.h>

#include "py/objproperty.h"
#include "py/runtime.h"
#include "shared-bindings/_bleio/Address.h"
#include "shared-bindings/_bleio/ScanEntry.h"
#include "shared-bindings/_bleio/UUID.h"
#include "shared-module/_bleio/ScanEntry.h"

//| class ScanEntry:
//|     """Encapsulates information about a device that was received during scanning. It can be
//|     advertisement or scan response data. This object may only be created by a `_bleio.ScanResults`:
//|     it has no user-visible constructor."""
//|

//|     def __init__(self) -> None:
//|         """Cannot be instantiated directly. Use `_bleio.Adapter.start_scan`."""
//|         ...
//|
//|     def matches(self, prefixes: ScanEntry, *, match_all: bool = True) -> bool:
//|         """Returns True if the ScanEntry matches all prefixes when ``match_all`` is True. This is stricter
//|         than the scan filtering which accepts any advertisements that match any of the prefixes
//|         where ``match_all`` is False."""
//|         ...
static mp_obj_t bleio_scanentry_matches(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    bleio_scanentry_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    enum { ARG_prefixes, ARG_match_all };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_prefixes, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_match_all, MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool = true} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    bool match_all = args[ARG_match_all].u_bool;

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_prefixes].u_obj, &bufinfo, MP_BUFFER_READ);
    return mp_obj_new_bool(common_hal_bleio_scanentry_matches(self, bufinfo.buf, bufinfo.len, match_all));
}
static MP_DEFINE_CONST_FUN_OBJ_KW(bleio_scanentry_matches_obj, 1, bleio_scanentry_matches);

//|     address: Address
//|     """The address of the device (read-only), of type `_bleio.Address`."""
static mp_obj_t bleio_scanentry_get_address(mp_obj_t self_in) {
    bleio_scanentry_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return common_hal_bleio_scanentry_get_address(self);
}
static MP_DEFINE_CONST_FUN_OBJ_1(bleio_scanentry_get_address_obj, bleio_scanentry_get_address);

MP_PROPERTY_GETTER(bleio_scanentry_address_obj,
    (mp_obj_t)&bleio_scanentry_get_address_obj);

//|     advertisement_bytes: bytes
//|     """All the advertisement data present in the packet, returned as a ``bytes`` object. (read-only)"""
static mp_obj_t scanentry_get_advertisement_bytes(mp_obj_t self_in) {
    bleio_scanentry_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return common_hal_bleio_scanentry_get_advertisement_bytes(self);
}
static MP_DEFINE_CONST_FUN_OBJ_1(bleio_scanentry_get_advertisement_bytes_obj, scanentry_get_advertisement_bytes);

MP_PROPERTY_GETTER(bleio_scanentry_advertisement_bytes_obj,
    (mp_obj_t)&bleio_scanentry_get_advertisement_bytes_obj);

//|     rssi: int
//|     """The signal strength of the device at the time of the scan, in integer dBm. (read-only)"""
static mp_obj_t scanentry_get_rssi(mp_obj_t self_in) {
    bleio_scanentry_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(common_hal_bleio_scanentry_get_rssi(self));
}
static MP_DEFINE_CONST_FUN_OBJ_1(bleio_scanentry_get_rssi_obj, scanentry_get_rssi);

MP_PROPERTY_GETTER(bleio_scanentry_rssi_obj,
    (mp_obj_t)&bleio_scanentry_get_rssi_obj);

//|     connectable: bool
//|     """True if the device can be connected to. (read-only)"""
static mp_obj_t scanentry_get_connectable(mp_obj_t self_in) {
    bleio_scanentry_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(common_hal_bleio_scanentry_get_connectable(self));
}
static MP_DEFINE_CONST_FUN_OBJ_1(bleio_scanentry_get_connectable_obj, scanentry_get_connectable);

MP_PROPERTY_GETTER(bleio_scanentry_connectable_obj,
    (mp_obj_t)&bleio_scanentry_get_connectable_obj);

//|     scan_response: bool
//|     """True if the entry was a scan response. (read-only)"""
//|
static mp_obj_t scanentry_get_scan_response(mp_obj_t self_in) {
    bleio_scanentry_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(common_hal_bleio_scanentry_get_scan_response(self));
}
static MP_DEFINE_CONST_FUN_OBJ_1(bleio_scanentry_get_scan_response_obj, scanentry_get_scan_response);

MP_PROPERTY_GETTER(bleio_scanentry_scan_response_obj,
    (mp_obj_t)&bleio_scanentry_get_scan_response_obj);

static const mp_rom_map_elem_t bleio_scanentry_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_address),             MP_ROM_PTR(&bleio_scanentry_address_obj) },
    { MP_ROM_QSTR(MP_QSTR_advertisement_bytes), MP_ROM_PTR(&bleio_scanentry_advertisement_bytes_obj) },
    { MP_ROM_QSTR(MP_QSTR_rssi),                MP_ROM_PTR(&bleio_scanentry_rssi_obj) },
    { MP_ROM_QSTR(MP_QSTR_connectable),         MP_ROM_PTR(&bleio_scanentry_connectable_obj) },
    { MP_ROM_QSTR(MP_QSTR_scan_response),       MP_ROM_PTR(&bleio_scanentry_scan_response_obj) },
    { MP_ROM_QSTR(MP_QSTR_matches),             MP_ROM_PTR(&bleio_scanentry_matches_obj) },
};

static MP_DEFINE_CONST_DICT(bleio_scanentry_locals_dict, bleio_scanentry_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    bleio_scanentry_type,
    MP_QSTR_ScanEntry,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    locals_dict, &bleio_scanentry_locals_dict
    );
