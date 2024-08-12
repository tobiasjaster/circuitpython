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
#include "shared-bindings/_bleio/Service.h"
#include "shared-bindings/_bleio/UUID.h"

//| class Characteristic:
//|     """Stores information about a BLE service characteristic and allows reading
//|     and writing of the characteristic's value."""
//|
//|     def __init__(self) -> None:
//|         """There is no regular constructor for a Characteristic. A new local Characteristic can be created
//|         and attached to a Service by calling `add_to_service()`.
//|         Remote Characteristic objects are created by `Connection.discover_remote_services()`
//|         as part of remote Services."""
//|         ...

//|     def add_to_service(
//|         self,
//|         service: Service,
//|         uuid: UUID,
//|         *,
//|         properties: int = 0,
//|         read_perm: int = Attribute.OPEN,
//|         write_perm: int = Attribute.OPEN,
//|         max_length: int = 20,
//|         fixed_length: bool = False,
//|         initial_value: Optional[ReadableBuffer] = None,
//|         user_description: Optional[str] = None
//|     ) -> Characteristic:
//|         """Create a new Characteristic object, and add it to this Service.
//|
//|         :param Service service: The service that will provide this characteristic
//|         :param UUID uuid: The uuid of the characteristic
//|         :param int properties: The properties of the characteristic,
//|            specified as a bitmask of these values bitwise-or'd together:
//|            `BROADCAST`, `INDICATE`, `NOTIFY`, `READ`, `WRITE`, `WRITE_NO_RESPONSE`.
//|         :param int read_perm: Specifies whether the characteristic can be read by a client, and if so, which
//|            security mode is required. Must be one of the integer values `Attribute.NO_ACCESS`, `Attribute.OPEN`,
//|            `Attribute.ENCRYPT_NO_MITM`, `Attribute.ENCRYPT_WITH_MITM`, `Attribute.LESC_ENCRYPT_WITH_MITM`,
//|            `Attribute.SIGNED_NO_MITM`, or `Attribute.SIGNED_WITH_MITM`.
//|         :param int write_perm: Specifies whether the characteristic can be written by a client, and if so, which
//|            security mode is required. Values allowed are the same as ``read_perm``.
//|         :param int max_length: Maximum length in bytes of the characteristic value. The maximum allowed is
//|          is 512, or possibly 510 if ``fixed_length`` is False. The default, 20, is the maximum
//|          number of data bytes that fit in a single BLE 4.x ATT packet.
//|         :param bool fixed_length: True if the characteristic value is of fixed length.
//|         :param ~circuitpython_typing.ReadableBuffer initial_value: The initial value for this characteristic. If not given, will be
//|          filled with zeros.
//|         :param str user_description: User friendly description of the characteristic
//|
//|         :return: the new Characteristic."""
//|         ...
static mp_obj_t bleio_characteristic_add_to_service(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // class is arg[0], which we can ignore.

    enum { ARG_service, ARG_uuid, ARG_properties, ARG_read_perm, ARG_write_perm,
           ARG_max_length, ARG_fixed_length, ARG_initial_value, ARG_user_description };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_service,  MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_uuid,  MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_properties, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_read_perm, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = SECURITY_MODE_OPEN} },
        { MP_QSTR_write_perm, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = SECURITY_MODE_OPEN} },
        { MP_QSTR_max_length, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 20} },
        { MP_QSTR_fixed_length, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_initial_value, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_user_description, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    bleio_service_obj_t *service = mp_arg_validate_type(args[ARG_service].u_obj, &bleio_service_type, MP_QSTR_service);

    bleio_uuid_obj_t *uuid = mp_arg_validate_type(args[ARG_uuid].u_obj, &bleio_uuid_type, MP_QSTR_uuid);

    const bleio_characteristic_properties_t properties = args[ARG_properties].u_int;
    if (properties & ~CHAR_PROP_ALL) {
        mp_arg_error_invalid(MP_QSTR_properties);
    }

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

    const char *user_description = NULL;
    if (args[ARG_user_description].u_obj != mp_const_none) {
        user_description = mp_obj_str_get_str(args[ARG_user_description].u_obj);
    }

    bleio_characteristic_obj_t *characteristic =
        mp_obj_malloc(bleio_characteristic_obj_t, &bleio_characteristic_type);

    // Range checking on max_length arg is done by the common_hal layer, because
    // it may vary depending on underlying BLE implementation.
    common_hal_bleio_characteristic_construct(
        characteristic, service, 0, uuid,
        properties, read_perm, write_perm,
        max_length, fixed_length, &initial_value_bufinfo,
        user_description);

    return MP_OBJ_FROM_PTR(characteristic);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(bleio_characteristic_add_to_service_fun_obj, 1, bleio_characteristic_add_to_service);
static MP_DEFINE_CONST_CLASSMETHOD_OBJ(bleio_characteristic_add_to_service_obj, MP_ROM_PTR(&bleio_characteristic_add_to_service_fun_obj));



//|     properties: int
//|     """An int bitmask representing which properties are set, specified as bitwise or'ing of
//|     of these possible values.
//|     `BROADCAST`, `INDICATE`, `NOTIFY`, `READ`, `WRITE`, `WRITE_NO_RESPONSE`."""
static mp_obj_t bleio_characteristic_get_properties(mp_obj_t self_in) {
    bleio_characteristic_obj_t *self = MP_OBJ_TO_PTR(self_in);

    return MP_OBJ_NEW_SMALL_INT(common_hal_bleio_characteristic_get_properties(self));
}
static MP_DEFINE_CONST_FUN_OBJ_1(bleio_characteristic_get_properties_obj, bleio_characteristic_get_properties);

MP_PROPERTY_GETTER(bleio_characteristic_properties_obj,
    (mp_obj_t)&bleio_characteristic_get_properties_obj);

//|     uuid: Optional[UUID]
//|     """The UUID of this characteristic. (read-only)
//|
//|     Will be ``None`` if the 128-bit UUID for this characteristic is not known."""
static mp_obj_t bleio_characteristic_get_uuid(mp_obj_t self_in) {
    bleio_characteristic_obj_t *self = MP_OBJ_TO_PTR(self_in);

    bleio_uuid_obj_t *uuid = common_hal_bleio_characteristic_get_uuid(self);
    return uuid ? MP_OBJ_FROM_PTR(uuid) : mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(bleio_characteristic_get_uuid_obj, bleio_characteristic_get_uuid);

MP_PROPERTY_GETTER(bleio_characteristic_uuid_obj,
    (mp_obj_t)&bleio_characteristic_get_uuid_obj);

//|     value: bytearray
//|     """The value of this characteristic."""
static mp_obj_t bleio_characteristic_get_value(mp_obj_t self_in) {
    bleio_characteristic_obj_t *self = MP_OBJ_TO_PTR(self_in);

    uint8_t temp[512];
    size_t actual_len = common_hal_bleio_characteristic_get_value(self, temp, sizeof(temp));
    return mp_obj_new_bytearray(actual_len, temp);
}
static MP_DEFINE_CONST_FUN_OBJ_1(bleio_characteristic_get_value_obj, bleio_characteristic_get_value);

static mp_obj_t bleio_characteristic_set_value(mp_obj_t self_in, mp_obj_t value_in) {
    bleio_characteristic_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(value_in, &bufinfo, MP_BUFFER_READ);

    common_hal_bleio_characteristic_set_value(self, &bufinfo);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(bleio_characteristic_set_value_obj, bleio_characteristic_set_value);

MP_PROPERTY_GETSET(bleio_characteristic_value_obj,
    (mp_obj_t)&bleio_characteristic_get_value_obj,
    (mp_obj_t)&bleio_characteristic_set_value_obj);

//|     max_length: int
//|     """The max length of this characteristic."""
static mp_obj_t bleio_characteristic_get_max_length(mp_obj_t self_in) {
    bleio_characteristic_obj_t *self = MP_OBJ_TO_PTR(self_in);

    return MP_OBJ_NEW_SMALL_INT(common_hal_bleio_characteristic_get_max_length(self));
}
static MP_DEFINE_CONST_FUN_OBJ_1(bleio_characteristic_get_max_length_obj, bleio_characteristic_get_max_length);

MP_PROPERTY_GETTER(bleio_characteristic_max_length_obj,
    (mp_obj_t)&bleio_characteristic_get_max_length_obj);

//|     descriptors: Descriptor
//|     """A tuple of :py:class:`Descriptor` objects related to this characteristic. (read-only)"""
static mp_obj_t bleio_characteristic_get_descriptors(mp_obj_t self_in) {
    bleio_characteristic_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // Return list as a tuple so user won't be able to change it.
    return MP_OBJ_FROM_PTR(common_hal_bleio_characteristic_get_descriptors(self));
}

static MP_DEFINE_CONST_FUN_OBJ_1(bleio_characteristic_get_descriptors_obj, bleio_characteristic_get_descriptors);

MP_PROPERTY_GETTER(bleio_characteristic_descriptors_obj,
    (mp_obj_t)&bleio_characteristic_get_descriptors_obj);

//|     service: Service
//|     """The Service this Characteristic is a part of."""
static mp_obj_t bleio_characteristic_get_service(mp_obj_t self_in) {
    bleio_characteristic_obj_t *self = MP_OBJ_TO_PTR(self_in);

    return common_hal_bleio_characteristic_get_service(self);
}
static MP_DEFINE_CONST_FUN_OBJ_1(bleio_characteristic_get_service_obj, bleio_characteristic_get_service);

MP_PROPERTY_GETTER(bleio_characteristic_service_obj,
    (mp_obj_t)&bleio_characteristic_get_service_obj);

//|     def set_cccd(self, *, notify: bool = False, indicate: bool = False) -> None:
//|         """Set the remote characteristic's CCCD to enable or disable notification and indication.
//|
//|         :param bool notify: True if Characteristic should receive notifications of remote writes
//|         :param float indicate: True if Characteristic should receive indications of remote writes
//|         """
//|         ...
static mp_obj_t bleio_characteristic_set_cccd(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    bleio_characteristic_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    enum { ARG_notify, ARG_indicate };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_notify, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_indicate, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    common_hal_bleio_characteristic_set_cccd(self, args[ARG_notify].u_bool, args[ARG_indicate].u_bool);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(bleio_characteristic_set_cccd_obj, 1, bleio_characteristic_set_cccd);

static const mp_rom_map_elem_t bleio_characteristic_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_add_to_service), MP_ROM_PTR(&bleio_characteristic_add_to_service_obj) },
    { MP_ROM_QSTR(MP_QSTR_descriptors),    MP_ROM_PTR(&bleio_characteristic_descriptors_obj) },
    { MP_ROM_QSTR(MP_QSTR_properties),     MP_ROM_PTR(&bleio_characteristic_properties_obj) },
    { MP_ROM_QSTR(MP_QSTR_uuid),           MP_ROM_PTR(&bleio_characteristic_uuid_obj) },
    { MP_ROM_QSTR(MP_QSTR_value),          MP_ROM_PTR(&bleio_characteristic_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_cccd),       MP_ROM_PTR(&bleio_characteristic_set_cccd_obj) },

    // Bitmask constants to represent properties
//|     BROADCAST: int
//|     """property: allowed in advertising packets"""
//|
//|     INDICATE: int
//|     """property: server will indicate to the client when the value is set and wait for a response"""
//|
//|     NOTIFY: int
//|     """property: server will notify the client when the value is set"""
//|
//|     READ: int
//|     """property: clients may read this characteristic"""
//|
//|     WRITE: int
//|     """property: clients may write this characteristic; a response will be sent back"""
//|
//|     WRITE_NO_RESPONSE: int
//|     """property: clients may write this characteristic; no response will be sent back"""
//|
    { MP_ROM_QSTR(MP_QSTR_BROADCAST),         MP_ROM_INT(CHAR_PROP_BROADCAST) },
    { MP_ROM_QSTR(MP_QSTR_INDICATE),          MP_ROM_INT(CHAR_PROP_INDICATE) },
    { MP_ROM_QSTR(MP_QSTR_NOTIFY),            MP_ROM_INT(CHAR_PROP_NOTIFY) },
    { MP_ROM_QSTR(MP_QSTR_READ),              MP_ROM_INT(CHAR_PROP_READ) },
    { MP_ROM_QSTR(MP_QSTR_WRITE),             MP_ROM_INT(CHAR_PROP_WRITE) },
    { MP_ROM_QSTR(MP_QSTR_WRITE_NO_RESPONSE), MP_ROM_INT(CHAR_PROP_WRITE_NO_RESPONSE) },

};
static MP_DEFINE_CONST_DICT(bleio_characteristic_locals_dict, bleio_characteristic_locals_dict_table);

static void bleio_characteristic_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    bleio_characteristic_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->uuid) {
        mp_printf(print, "Characteristic(");
        bleio_uuid_print(print, MP_OBJ_FROM_PTR(self->uuid), kind);
        mp_printf(print, ")");
    } else {
        mp_printf(print, "<Characteristic with Unregistered UUID>");
    }
}

MP_DEFINE_CONST_OBJ_TYPE(
    bleio_characteristic_type,
    MP_QSTR_Characteristic,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    print, bleio_characteristic_print,
    locals_dict, &bleio_characteristic_locals_dict
    );
