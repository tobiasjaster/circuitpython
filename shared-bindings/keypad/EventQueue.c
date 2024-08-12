// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 Dan Halbert for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "py/stream.h"
#include "py/mperrno.h"
#include "py/objproperty.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "shared-bindings/keypad/Event.h"
#include "shared-bindings/keypad/EventQueue.h"

//| class EventQueue:
//|     """A queue of `Event` objects, filled by a `keypad` scanner such as `Keys` or `KeyMatrix`.
//|
//|     You cannot create an instance of `EventQueue` directly. Each scanner creates an
//|     instance when it is created.
//|     """
//|
//|     ...

//|     def get(self) -> Optional[Event]:
//|         """Return the next key transition event. Return ``None`` if no events are pending.
//|
//|         Note that the queue size is limited; see ``max_events`` in the constructor of
//|         a scanner such as `Keys` or `KeyMatrix`.
//|         If a new event arrives when the queue is full, the event is discarded, and
//|         `overflowed` is set to ``True``.
//|
//|         :return: The next queued key transition `Event`.
//|         :rtype: Optional[Event]
//|         """
//|         ...
static mp_obj_t keypad_eventqueue_get(mp_obj_t self_in) {
    keypad_eventqueue_obj_t *self = MP_OBJ_TO_PTR(self_in);

    return common_hal_keypad_eventqueue_get(self);
}
MP_DEFINE_CONST_FUN_OBJ_1(keypad_eventqueue_get_obj, keypad_eventqueue_get);

//|     def get_into(self, event: Event) -> bool:
//|         """Store the next key transition event in the supplied event, if available,
//|         and return ``True``.
//|         If there are no queued events, do not touch ``event`` and return ``False``.
//|
//|         The advantage of this method over ``get()`` is that it does not allocate storage.
//|         Instead you can reuse an existing ``Event`` object.
//|
//|         Note that the queue size is limited; see ``max_events`` in the constructor of
//|         a scanner such as `Keys` or `KeyMatrix`.
//|
//|         :return: ``True`` if an event was available and stored, ``False`` if not.
//|         :rtype: bool
//|         """
//|         ...
static mp_obj_t keypad_eventqueue_get_into(mp_obj_t self_in, mp_obj_t event_in) {
    keypad_eventqueue_obj_t *self = MP_OBJ_TO_PTR(self_in);

    keypad_event_obj_t *event = MP_OBJ_TO_PTR(mp_arg_validate_type(event_in, &keypad_event_type, MP_QSTR_event));

    return mp_obj_new_bool(common_hal_keypad_eventqueue_get_into(self, event));
}
MP_DEFINE_CONST_FUN_OBJ_2(keypad_eventqueue_get_into_obj, keypad_eventqueue_get_into);

//|     def clear(self) -> None:
//|         """Clear any queued key transition events. Also sets `overflowed` to ``False``."""
//|         ...
static mp_obj_t keypad_eventqueue_clear(mp_obj_t self_in) {
    keypad_eventqueue_obj_t *self = MP_OBJ_TO_PTR(self_in);

    common_hal_keypad_eventqueue_clear(self);
    return MP_ROM_NONE;
}
MP_DEFINE_CONST_FUN_OBJ_1(keypad_eventqueue_clear_obj, keypad_eventqueue_clear);

//|     def __bool__(self) -> bool:
//|         """``True`` if `len()` is greater than zero.
//|         This is an easy way to check if the queue is empty.
//|         """
//|         ...
//|
//|     def __len__(self) -> int:
//|         """Return the number of events currently in the queue. Used to implement ``len()``."""
//|         ...
static mp_obj_t keypad_eventqueue_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    keypad_eventqueue_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint16_t len = common_hal_keypad_eventqueue_get_length(self);
    switch (op) {
        case MP_UNARY_OP_BOOL:
            return mp_obj_new_bool(len != 0);
        case MP_UNARY_OP_LEN:
            return MP_OBJ_NEW_SMALL_INT(len);
        default:
            return MP_OBJ_NULL;      // op not supported
    }
}

//|     overflowed: bool
//|     """``True`` if an event could not be added to the event queue because it was full. (read-only)
//|     Set to ``False`` by  `clear()`.
//|     """
//|
static mp_obj_t keypad_eventqueue_get_overflowed(mp_obj_t self_in) {
    keypad_eventqueue_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(common_hal_keypad_eventqueue_get_overflowed(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(keypad_eventqueue_get_overflowed_obj, keypad_eventqueue_get_overflowed);

MP_PROPERTY_GETTER(keypad_eventqueue_overflowed_obj,
    (mp_obj_t)&keypad_eventqueue_get_overflowed_obj);

static const mp_rom_map_elem_t keypad_eventqueue_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_clear),      MP_ROM_PTR(&keypad_eventqueue_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_get),        MP_ROM_PTR(&keypad_eventqueue_get_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_into),   MP_ROM_PTR(&keypad_eventqueue_get_into_obj) },
    { MP_ROM_QSTR(MP_QSTR_overflowed), MP_ROM_PTR(&keypad_eventqueue_overflowed_obj) },
};

static MP_DEFINE_CONST_DICT(keypad_eventqueue_locals_dict, keypad_eventqueue_locals_dict_table);

#if MICROPY_PY_SELECT
static mp_uint_t eventqueue_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    (void)errcode;
    keypad_eventqueue_obj_t *self = MP_OBJ_TO_PTR(self_in);
    switch (request) {
        case MP_STREAM_POLL: {
            mp_uint_t flags = arg;
            mp_uint_t ret = 0;
            if ((flags & MP_STREAM_POLL_RD) && common_hal_keypad_eventqueue_get_length(self)) {
                ret |= MP_STREAM_POLL_RD;
            }
            return ret;
        }
        default:
            *errcode = MP_EINVAL;
            return MP_STREAM_ERROR;
    }
}

static const mp_stream_p_t eventqueue_p = {
    .ioctl = eventqueue_ioctl,
};
#endif


MP_DEFINE_CONST_OBJ_TYPE(
    keypad_eventqueue_type,
    MP_QSTR_EventQueue,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    unary_op, keypad_eventqueue_unary_op,
    #if MICROPY_PY_SELECT
    protocol, &eventqueue_p,
    #endif
    locals_dict, &keypad_eventqueue_locals_dict
    );
