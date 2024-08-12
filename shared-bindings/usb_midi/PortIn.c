// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2018 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "shared-bindings/usb_midi/PortIn.h"
#include "shared-bindings/util.h"

#include "py/stream.h"
#include "py/objproperty.h"
#include "py/runtime.h"
#include "py/stream.h"

//| class PortIn:
//|     """Receives midi commands over USB"""
//|
//|     def __init__(self) -> None:
//|         """You cannot create an instance of `usb_midi.PortIn`.
//|
//|         PortIn objects are constructed for every corresponding entry in the USB
//|         descriptor and added to the ``usb_midi.ports`` tuple."""
//|         ...

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
//|     def readinto(self, buf: WriteableBuffer, nbytes: Optional[int] = None) -> Optional[bytes]:
//|         """Read bytes into the ``buf``.  If ``nbytes`` is specified then read at most
//|         that many bytes.  Otherwise, read at most ``len(buf)`` bytes.
//|
//|         :return: number of bytes read and stored into ``buf``
//|         :rtype: bytes or None"""
//|         ...
//|

// These three methods are used by the shared stream methods.
static mp_uint_t usb_midi_portin_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    usb_midi_portin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    byte *buf = buf_in;

    // make sure we want at least 1 char
    if (size == 0) {
        return 0;
    }

    return common_hal_usb_midi_portin_read(self, buf, size, errcode);
}

static mp_uint_t usb_midi_portin_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode) {
    usb_midi_portin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_uint_t ret;
    if (request == MP_STREAM_POLL) {
        mp_uint_t flags = arg;
        ret = 0;
        if ((flags & MP_STREAM_POLL_RD) && common_hal_usb_midi_portin_bytes_available(self) > 0) {
            ret |= MP_STREAM_POLL_RD;
        }
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}

static const mp_rom_map_elem_t usb_midi_portin_locals_dict_table[] = {
    // Standard stream methods.
    { MP_OBJ_NEW_QSTR(MP_QSTR_read),     MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
};
static MP_DEFINE_CONST_DICT(usb_midi_portin_locals_dict, usb_midi_portin_locals_dict_table);

static const mp_stream_p_t usb_midi_portin_stream_p = {
    .read = usb_midi_portin_read,
    .write = NULL,
    .ioctl = usb_midi_portin_ioctl,
    .is_text = false,
};

MP_DEFINE_CONST_OBJ_TYPE(
    usb_midi_portin_type,
    MP_QSTR_PortIn,
    MP_TYPE_FLAG_ITER_IS_ITERNEXT,
    locals_dict, &usb_midi_portin_locals_dict,
    iter, mp_stream_unbuffered_iter,
    protocol, &usb_midi_portin_stream_p
    );
