// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Dan Halbert for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2018 Artur Pacholec
//
// SPDX-License-Identifier: MIT

#include <string.h>
#include <stdio.h>

#include "py/objproperty.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "shared-bindings/_bleio/Address.h"
#include "shared-module/_bleio/Address.h"

//| class Address:
//|     """Encapsulates the address of a BLE device."""
//|

//|     def __init__(self, address: ReadableBuffer, address_type: int) -> None:
//|         """Create a new Address object encapsulating the address value.
//|         The value itself can be one of:
//|
//|         :param ~circuitpython_typing.ReadableBuffer address: The address value to encapsulate. A buffer object (bytearray, bytes) of 6 bytes.
//|         :param int address_type: one of the integer values: `PUBLIC`, `RANDOM_STATIC`,
//|           `RANDOM_PRIVATE_RESOLVABLE`, or `RANDOM_PRIVATE_NON_RESOLVABLE`."""
//|         ...
static mp_obj_t bleio_address_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_address, ARG_address_type };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_address, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_address_type, MP_ARG_INT, {.u_int = BLEIO_ADDRESS_TYPE_PUBLIC } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    bleio_address_obj_t *self = mp_obj_malloc(bleio_address_obj_t, &bleio_address_type);

    const mp_obj_t address = args[ARG_address].u_obj;
    mp_buffer_info_t buf_info;
    mp_get_buffer_raise(address, &buf_info, MP_BUFFER_READ);
    if (buf_info.len != NUM_BLEIO_ADDRESS_BYTES) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("Address must be %d bytes long"), NUM_BLEIO_ADDRESS_BYTES);
    }

    const mp_int_t address_type = args[ARG_address_type].u_int;
    if (address_type < BLEIO_ADDRESS_TYPE_MIN || address_type > BLEIO_ADDRESS_TYPE_MAX) {
        mp_arg_error_invalid(MP_QSTR_address_type);
    }

    common_hal_bleio_address_construct(self, buf_info.buf, address_type);

    return MP_OBJ_FROM_PTR(self);
}

//|     address_bytes: bytes
//|     """The bytes that make up the device address (read-only).
//|
//|     Note that the ``bytes`` object returned is in little-endian order:
//|     The least significant byte is ``address_bytes[0]``. So the address will
//|     appear to be reversed if you print the raw ``bytes`` object. If you print
//|     or use `str()` on the :py:class:`~_bleio.Attribute` object itself, the address will be printed
//|     in the expected order. For example:
//|
//|     .. code-block:: python
//|
//|       >>> import _bleio
//|       >>> _bleio.adapter.address
//|       <Address c8:1d:f5:ed:a8:35>
//|       >>> _bleio.adapter.address.address_bytes
//|       b'5\\xa8\\xed\\xf5\\x1d\\xc8'"""
static mp_obj_t bleio_address_get_address_bytes(mp_obj_t self_in) {
    bleio_address_obj_t *self = MP_OBJ_TO_PTR(self_in);

    return common_hal_bleio_address_get_address_bytes(self);
}
MP_DEFINE_CONST_FUN_OBJ_1(bleio_address_get_address_bytes_obj, bleio_address_get_address_bytes);

MP_PROPERTY_GETTER(bleio_address_address_bytes_obj,
    (mp_obj_t)&bleio_address_get_address_bytes_obj);

//|     type: int
//|     """The address type (read-only).
//|
//|     One of the integer values: `PUBLIC`, `RANDOM_STATIC`, `RANDOM_PRIVATE_RESOLVABLE`,
//|     or `RANDOM_PRIVATE_NON_RESOLVABLE`."""
static mp_obj_t bleio_address_get_type(mp_obj_t self_in) {
    bleio_address_obj_t *self = MP_OBJ_TO_PTR(self_in);

    return MP_OBJ_NEW_SMALL_INT(common_hal_bleio_address_get_type(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(bleio_address_get_type_obj, bleio_address_get_type);

MP_PROPERTY_GETTER(bleio_address_type_obj,
    (mp_obj_t)&bleio_address_get_type_obj);

//|     def __eq__(self, other: object) -> bool:
//|         """Two Address objects are equal if their addresses and address types are equal."""
//|         ...
static mp_obj_t bleio_address_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in) {
    switch (op) {
        // Two Addresses are equal if their address bytes and address_type are equal
        case MP_BINARY_OP_EQUAL:
            if (mp_obj_is_type(rhs_in, &bleio_address_type)) {
                bleio_address_obj_t *lhs = MP_OBJ_TO_PTR(lhs_in);
                bleio_address_obj_t *rhs = MP_OBJ_TO_PTR(rhs_in);
                return mp_obj_new_bool(
                    mp_obj_equal(common_hal_bleio_address_get_address_bytes(lhs),
                        common_hal_bleio_address_get_address_bytes(rhs)) &&
                    common_hal_bleio_address_get_type(lhs) ==
                    common_hal_bleio_address_get_type(rhs));

            } else {
                return mp_const_false;
            }

        default:
            return MP_OBJ_NULL; // op not supported
    }
}

//|     def __hash__(self) -> int:
//|         """Returns a hash for the Address data."""
//|         ...
static mp_obj_t bleio_address_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    switch (op) {
        // Two Addresses are equal if their address bytes and address_type are equal
        case MP_UNARY_OP_HASH: {
            mp_obj_t bytes = common_hal_bleio_address_get_address_bytes(MP_OBJ_TO_PTR(self_in));
            GET_STR_HASH(bytes, h);
            if (h == 0) {
                GET_STR_DATA_LEN(bytes, data, len);
                h = qstr_compute_hash(data, len);
            }
            h ^= common_hal_bleio_address_get_type(MP_OBJ_TO_PTR(self_in));
            return MP_OBJ_NEW_SMALL_INT(h);
        }
        default:
            return MP_OBJ_NULL; // op not supported
    }
}

static void bleio_address_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    bleio_address_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_buffer_info_t buf_info;
    mp_obj_t address_bytes = common_hal_bleio_address_get_address_bytes(self);
    mp_get_buffer_raise(address_bytes, &buf_info, MP_BUFFER_READ);

    const uint8_t *buf = (uint8_t *)buf_info.buf;
    mp_printf(print, "<Address %02x:%02x:%02x:%02x:%02x:%02x>",
        buf[5], buf[4], buf[3], buf[2], buf[1], buf[0]);
}

//|     PUBLIC: int
//|     """A publicly known address, with a company ID (high 24 bits)and company-assigned part (low 24 bits)."""
//|
//|     RANDOM_STATIC: int
//|     """A randomly generated address that does not change often. It may never change or may change after
//|      a power cycle."""
//|
//|     RANDOM_PRIVATE_RESOLVABLE: int
//|     """An address that is usable when the peer knows the other device's secret Identity Resolving Key (IRK)."""
//|
//|     RANDOM_PRIVATE_NON_RESOLVABLE: int
//|     """A randomly generated address that changes on every connection."""
//|
static const mp_rom_map_elem_t bleio_address_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_address_bytes),                 MP_ROM_PTR(&bleio_address_address_bytes_obj) },
    { MP_ROM_QSTR(MP_QSTR_type),                          MP_ROM_PTR(&bleio_address_type_obj) },
    // These match the BLE_GAP_ADDR_TYPES values used by the nRF library.
    { MP_ROM_QSTR(MP_QSTR_PUBLIC),                        MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_RANDOM_STATIC),                 MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_RANDOM_PRIVATE_RESOLVABLE),     MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_RANDOM_PRIVATE_NON_RESOLVABLE), MP_ROM_INT(3) },

};

static MP_DEFINE_CONST_DICT(bleio_address_locals_dict, bleio_address_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    bleio_address_type,
    MP_QSTR_Address,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    make_new, bleio_address_make_new,
    print, bleio_address_print,
    locals_dict, &bleio_address_locals_dict,
    unary_op, bleio_address_unary_op,
    binary_op, bleio_address_binary_op
    );
