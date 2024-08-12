// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2018 Dan Halbert for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "py/mperrno.h"
#include "py/stream.h"
#include "py/objproperty.h"
#include "py/runtime.h"
#include "py/stream.h"

#include "shared-bindings/_bleio/__init__.h"
#include "shared-bindings/_bleio/CharacteristicBuffer.h"
#include "shared-bindings/_bleio/UUID.h"
#include "shared-bindings/util.h"

static void raise_error_if_not_connected(bleio_characteristic_buffer_obj_t *self) {
    if (!common_hal_bleio_characteristic_buffer_connected(self)) {
        mp_raise_ConnectionError(MP_ERROR_TEXT("Not connected"));
    }
}

//| class CharacteristicBuffer:
//|     """Accumulates a Characteristic's incoming values in a FIFO buffer."""
//|
//|     def __init__(
//|         self, characteristic: Characteristic, *, timeout: int = 1, buffer_size: int = 64
//|     ) -> None:
//|         """Monitor the given Characteristic. Each time a new value is written to the Characteristic
//|         add the newly-written bytes to a FIFO buffer.
//|
//|         :param Characteristic characteristic: The Characteristic to monitor.
//|           It may be a local Characteristic provided by a Peripheral Service, or a remote Characteristic
//|           in a remote Service that a Central has connected to.
//|         :param int timeout:  the timeout in seconds to wait for the first character and between subsequent characters.
//|         :param int buffer_size: Size of ring buffer that stores incoming data coming from client.
//|           Must be >= 1."""
//|         ...
static mp_obj_t bleio_characteristic_buffer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_characteristic, ARG_timeout, ARG_buffer_size, };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_characteristic,  MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_SMALL_INT(1)} },
        { MP_QSTR_buffer_size, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 64} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    bleio_characteristic_obj_t *characteristic = mp_arg_validate_type(args[ARG_characteristic].u_obj, &bleio_characteristic_type, MP_QSTR_characteristic);

    mp_float_t timeout = mp_arg_validate_obj_float_non_negative(args[ARG_timeout].u_obj, 1.0f, MP_QSTR_timeout);

    const mp_int_t buffer_size = mp_arg_validate_int_min(args[ARG_buffer_size].u_int, 1, MP_QSTR_buffer_size);

    bleio_characteristic_buffer_obj_t *self =
        mp_obj_malloc(bleio_characteristic_buffer_obj_t, &bleio_characteristic_buffer_type);

    common_hal_bleio_characteristic_buffer_construct(self, characteristic, timeout, buffer_size);

    return MP_OBJ_FROM_PTR(self);
}

static void check_for_deinit(bleio_characteristic_buffer_obj_t *self) {
    if (common_hal_bleio_characteristic_buffer_deinited(self)) {
        raise_deinited_error();
    }
}

// These are standard stream methods. Code is in py/stream.c.
//
//|     def read(self, nbytes: Optional[int] = None) -> Optional[bytes]:
//|         """Read characters.  If ``nbytes`` is specified then read at most that many
//|         bytes. Otherwise, read everything that arrives until the connection
//|         times out. Providing the number of bytes expected is highly recommended
//|         because it will be faster.
//|
//|         :return: Data read
//|         :rtype: bytes or None"""
//|         ...
//|
//|     def readinto(self, buf: WriteableBuffer) -> Optional[int]:
//|         """Read bytes into the ``buf``. Read at most ``len(buf)`` bytes.
//|
//|         :return: number of bytes read and stored into ``buf``
//|         :rtype: int or None (on a non-blocking error)"""
//|         ...
//|
//|     def readline(self) -> bytes:
//|         """Read a line, ending in a newline character.
//|
//|         :return: the line read
//|         :rtype: int or None"""
//|         ...

// These three methods are used by the shared stream methods.
static mp_uint_t bleio_characteristic_buffer_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    bleio_characteristic_buffer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);
    raise_error_if_not_connected(self);
    byte *buf = buf_in;

    // make sure we want at least 1 char
    if (size == 0) {
        return 0;
    }

    return common_hal_bleio_characteristic_buffer_read(self, buf, size, errcode);
}

static mp_uint_t bleio_characteristic_buffer_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
    mp_raise_NotImplementedError(MP_ERROR_TEXT("CharacteristicBuffer writing not provided"));
    return 0;
}

static mp_uint_t bleio_characteristic_buffer_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode) {
    bleio_characteristic_buffer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);
    raise_error_if_not_connected(self);
    mp_uint_t ret;
    if (request == MP_STREAM_POLL) {
        mp_uint_t flags = arg;
        ret = 0;
        if ((flags & MP_STREAM_POLL_RD) && common_hal_bleio_characteristic_buffer_rx_characters_available(self) > 0) {
            ret |= MP_STREAM_POLL_RD;
        }
// No writing provided.
//        if ((flags & MP_STREAM_POLL_WR) && common_hal_busio_uart_ready_to_tx(self)) {
//            ret |= MP_STREAM_POLL_WR;
//        }
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}

//|     in_waiting: int
//|     """The number of bytes in the input buffer, available to be read"""
static mp_obj_t bleio_characteristic_buffer_obj_get_in_waiting(mp_obj_t self_in) {
    bleio_characteristic_buffer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);
    uint32_t available = common_hal_bleio_characteristic_buffer_rx_characters_available(self);
    if (available == 0) {
        // Only check if connected when none available, otherwise, allow code to continue.
        raise_error_if_not_connected(self);
    }
    return MP_OBJ_NEW_SMALL_INT(available);
}
MP_DEFINE_CONST_FUN_OBJ_1(bleio_characteristic_buffer_get_in_waiting_obj, bleio_characteristic_buffer_obj_get_in_waiting);

MP_PROPERTY_GETTER(bleio_characteristic_buffer_in_waiting_obj,
    (mp_obj_t)&bleio_characteristic_buffer_get_in_waiting_obj);

//|     def reset_input_buffer(self) -> None:
//|         """Discard any unread characters in the input buffer."""
//|         ...
static mp_obj_t bleio_characteristic_buffer_obj_reset_input_buffer(mp_obj_t self_in) {
    bleio_characteristic_buffer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);
    common_hal_bleio_characteristic_buffer_clear_rx_buffer(self);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(bleio_characteristic_buffer_reset_input_buffer_obj, bleio_characteristic_buffer_obj_reset_input_buffer);

//|     def deinit(self) -> None:
//|         """Disable permanently."""
//|         ...
//|
static mp_obj_t bleio_characteristic_buffer_deinit(mp_obj_t self_in) {
    bleio_characteristic_buffer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_bleio_characteristic_buffer_deinit(self);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(bleio_characteristic_buffer_deinit_obj, bleio_characteristic_buffer_deinit);

static const mp_rom_map_elem_t bleio_characteristic_buffer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_deinit),        MP_ROM_PTR(&bleio_characteristic_buffer_deinit_obj) },

    // Standard stream methods.
    { MP_OBJ_NEW_QSTR(MP_QSTR_read),     MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj)},
    { MP_OBJ_NEW_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    // CharacteristicBuffer is currently read-only.
    // { MP_OBJ_NEW_QSTR(MP_QSTR_write),    MP_ROM_PTR(&mp_stream_write_obj) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_reset_input_buffer), MP_ROM_PTR(&bleio_characteristic_buffer_reset_input_buffer_obj) },
    // Properties
    { MP_ROM_QSTR(MP_QSTR_in_waiting), MP_ROM_PTR(&bleio_characteristic_buffer_in_waiting_obj) },

};

static MP_DEFINE_CONST_DICT(bleio_characteristic_buffer_locals_dict, bleio_characteristic_buffer_locals_dict_table);

static const mp_stream_p_t characteristic_buffer_stream_p = {
    .read = bleio_characteristic_buffer_read,
    .write = bleio_characteristic_buffer_write,
    .ioctl = bleio_characteristic_buffer_ioctl,
    .is_text = false,
    // Disallow readinto() size parameter.
    .pyserial_readinto_compatibility = true,
};


MP_DEFINE_CONST_OBJ_TYPE(
    bleio_characteristic_buffer_type,
    MP_QSTR_CharacteristicBuffer,
    MP_TYPE_FLAG_ITER_IS_ITERNEXT | MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    make_new, bleio_characteristic_buffer_make_new,
    locals_dict, &bleio_characteristic_buffer_locals_dict,
    iter, mp_stream_unbuffered_iter,
    protocol, &characteristic_buffer_stream_p
    );
