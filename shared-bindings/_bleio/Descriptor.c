// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Dan Halbert for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2018 Artur Pacholec
// SPDX-FileCopyrightText: Copyright (c) 2017 Glenn Ruben Bakke
//
// SPDX-License-Identifier: MIT

#include "py/objproperty.h"
#include "py/runtime.h"
#include "shared-bindings/_bleio/Attribute.h"
#include "shared-bindings/_bleio/Characteristic.h"
#include "shared-bindings/_bleio/Descriptor.h"
#include "shared-bindings/_bleio/UUID.h"

//| class Descriptor:
//|     """Stores information about a BLE descriptor.
//|
//|     Descriptors are attached to BLE characteristics and provide contextual
//|     information about the characteristic."""
//|
//|     def __init__(self) -> None:
//|         """There is no regular constructor for a Descriptor. A new local Descriptor can be created
//|         and attached to a Characteristic by calling `add_to_characteristic()`.
//|         Remote Descriptor objects are created by `Connection.discover_remote_services()`
//|         as part of remote Characteristics in the remote Services that are discovered."""
//|
//|     @classmethod
//|     def add_to_characteristic(
//|         cls,
//|         characteristic: Characteristic,
//|         uuid: UUID,
//|         *,
//|         read_perm: int = Attribute.OPEN,
//|         write_perm: int = Attribute.OPEN,
//|         max_length: int = 20,
//|         fixed_length: bool = False,
//|         initial_value: ReadableBuffer = b""
//|     ) -> Descriptor:
//|         """Create a new Descriptor object, and add it to this Service.
//|
//|         :param Characteristic characteristic: The characteristic that will hold this descriptor
//|         :param UUID uuid: The uuid of the descriptor
//|         :param int read_perm: Specifies whether the descriptor can be read by a client, and if so, which
//|            security mode is required. Must be one of the integer values `Attribute.NO_ACCESS`, `Attribute.OPEN`,
//|            `Attribute.ENCRYPT_NO_MITM`, `Attribute.ENCRYPT_WITH_MITM`, `Attribute.LESC_ENCRYPT_WITH_MITM`,
//|            `Attribute.SIGNED_NO_MITM`, or `Attribute.SIGNED_WITH_MITM`.
//|         :param int write_perm: Specifies whether the descriptor can be written by a client, and if so, which
//|            security mode is required. Values allowed are the same as ``read_perm``.
//|         :param int max_length: Maximum length in bytes of the descriptor value. The maximum allowed is
//|            is 512, or possibly 510 if ``fixed_length`` is False. The default, 20, is the maximum
//|            number of data bytes that fit in a single BLE 4.x ATT packet.
//|         :param bool fixed_length: True if the descriptor value is of fixed length.
//|         :param ~circuitpython_typing.ReadableBuffer initial_value: The initial value for this descriptor.
//|
//|         :return: the new Descriptor."""
//|         ...
static mp_obj_t bleio_descriptor_add_to_characteristic(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // class is arg[0], which we can ignore.

    enum { ARG_characteristic, ARG_uuid, ARG_read_perm, ARG_write_perm,
           ARG_max_length, ARG_fixed_length, ARG_initial_value };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_characteristic,  MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_uuid,  MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_read_perm, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = SECURITY_MODE_OPEN} },
        { MP_QSTR_write_perm, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = SECURITY_MODE_OPEN} },
        { MP_QSTR_max_length, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 20} },
        { MP_QSTR_fixed_length, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_initial_value, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_empty_bytes} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    bleio_characteristic_obj_t *characteristic = mp_arg_validate_type(args[ARG_characteristic].u_obj, &bleio_characteristic_type, MP_QSTR_characteristic);

    bleio_uuid_obj_t *uuid = mp_arg_validate_type(args[ARG_uuid].u_obj, &bleio_uuid_type, MP_QSTR_uuid);

    const bleio_attribute_security_mode_t read_perm = args[ARG_read_perm].u_int;
    common_hal_bleio_attribute_security_mode_check_valid(read_perm);

    const bleio_attribute_security_mode_t write_perm = args[ARG_write_perm].u_int;
    common_hal_bleio_attribute_security_mode_check_valid(write_perm);

    const mp_int_t max_length_int = mp_arg_validate_int_min(args[ARG_max_length].u_int, 0, MP_QSTR_max_length);

    const size_t max_length = (size_t)max_length_int;
    const bool fixed_length = args[ARG_fixed_length].u_bool;
    mp_obj_t initial_value = args[ARG_initial_value].u_obj;

    mp_buffer_info_t initial_value_bufinfo;
    if (initial_value == mp_const_none) {
        if (fixed_length && max_length > 0) {
            initial_value = mp_obj_new_bytes_of_zeros(max_length);
        } else {
            initial_value = mp_const_empty_bytes;
        }
    }
    mp_get_buffer_raise(initial_value, &initial_value_bufinfo, MP_BUFFER_READ);
    if (initial_value_bufinfo.len > max_length ||
        (fixed_length && initial_value_bufinfo.len != max_length)) {
        mp_raise_ValueError(MP_ERROR_TEXT("initial_value length is wrong"));
    }

    bleio_descriptor_obj_t *descriptor = mp_obj_malloc(bleio_descriptor_obj_t, &bleio_descriptor_type);

    // Range checking on max_length arg is done by the common_hal layer, because
    // it may vary depending on underlying BLE implementation.
    common_hal_bleio_descriptor_construct(
        descriptor, characteristic, uuid,
        read_perm, write_perm,
        max_length, fixed_length, &initial_value_bufinfo);

    common_hal_bleio_characteristic_add_descriptor(characteristic, descriptor);

    return MP_OBJ_FROM_PTR(descriptor);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(bleio_descriptor_add_to_characteristic_fun_obj, 1, bleio_descriptor_add_to_characteristic);
static MP_DEFINE_CONST_CLASSMETHOD_OBJ(bleio_descriptor_add_to_characteristic_obj, MP_ROM_PTR(&bleio_descriptor_add_to_characteristic_fun_obj));

//|     uuid: UUID
//|     """The descriptor uuid. (read-only)"""
static mp_obj_t bleio_descriptor_get_uuid(mp_obj_t self_in) {
    bleio_descriptor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    bleio_uuid_obj_t *uuid = common_hal_bleio_descriptor_get_uuid(self);
    return uuid ? MP_OBJ_FROM_PTR(uuid) : mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(bleio_descriptor_get_uuid_obj, bleio_descriptor_get_uuid);

MP_PROPERTY_GETTER(bleio_descriptor_uuid_obj,
    (mp_obj_t)&bleio_descriptor_get_uuid_obj);

//|     characteristic: Characteristic
//|     """The Characteristic this Descriptor is a part of."""
static mp_obj_t bleio_descriptor_get_characteristic(mp_obj_t self_in) {
    bleio_descriptor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    return common_hal_bleio_descriptor_get_characteristic(self);
}
static MP_DEFINE_CONST_FUN_OBJ_1(bleio_descriptor_get_characteristic_obj, bleio_descriptor_get_characteristic);

MP_PROPERTY_GETTER(bleio_descriptor_characteristic_obj,
    (mp_obj_t)&bleio_descriptor_get_characteristic_obj);

//|     value: bytearray
//|     """The value of this descriptor."""
//|
static mp_obj_t bleio_descriptor_get_value(mp_obj_t self_in) {
    bleio_descriptor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    uint8_t temp[512];
    size_t actual_len = common_hal_bleio_descriptor_get_value(self, temp, sizeof(temp));
    return mp_obj_new_bytearray(actual_len, temp);
}
static MP_DEFINE_CONST_FUN_OBJ_1(bleio_descriptor_get_value_obj, bleio_descriptor_get_value);

static mp_obj_t bleio_descriptor_set_value(mp_obj_t self_in, mp_obj_t value_in) {
    bleio_descriptor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(value_in, &bufinfo, MP_BUFFER_READ);

    common_hal_bleio_descriptor_set_value(self, &bufinfo);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(bleio_descriptor_set_value_obj, bleio_descriptor_set_value);

MP_PROPERTY_GETSET(bleio_descriptor_value_obj,
    (mp_obj_t)&bleio_descriptor_get_value_obj,
    (mp_obj_t)&bleio_descriptor_set_value_obj);

static const mp_rom_map_elem_t bleio_descriptor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_add_to_characteristic), MP_ROM_PTR(&bleio_descriptor_add_to_characteristic_obj) },
    { MP_ROM_QSTR(MP_QSTR_uuid), MP_ROM_PTR(&bleio_descriptor_uuid_obj) },
    { MP_ROM_QSTR(MP_QSTR_characteristic), MP_ROM_PTR(&bleio_descriptor_characteristic_obj) },
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&bleio_descriptor_value_obj) },
};

static MP_DEFINE_CONST_DICT(bleio_descriptor_locals_dict, bleio_descriptor_locals_dict_table);

static void bleio_descriptor_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    bleio_descriptor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->uuid) {
        mp_printf(print, "Descriptor(");
        bleio_uuid_print(print, MP_OBJ_FROM_PTR(self->uuid), kind);
        mp_printf(print, ")");
    } else {
        mp_printf(print, "<Descriptor with Unregistered UUID>");
    }
}

MP_DEFINE_CONST_OBJ_TYPE(
    bleio_descriptor_type,
    MP_QSTR_Descriptor,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    print, bleio_descriptor_print,
    locals_dict, &bleio_descriptor_locals_dict
    );
