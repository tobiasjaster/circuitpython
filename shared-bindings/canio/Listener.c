// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Jeff Epler for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/canio/Listener.h"
#include "shared-bindings/canio/Message.h"
#include "common-hal/canio/Listener.h"

#include "py/runtime.h"
#include "py/objproperty.h"

//| class Listener:
//|     """Listens for CAN message
//|
//|     `canio.Listener` is not constructed directly, but instead by calling
//|     `canio.CAN.listen`.
//|
//|     In addition to using the `receive` method to retrieve a message or
//|     the `in_waiting` method to check for an available message, a
//|     listener can be used as an iterable, yielding messages until no
//|     message arrives within ``self.timeout`` seconds."""
//|

//|     def receive(self) -> Optional[Union[RemoteTransmissionRequest, Message]]:
//|         """Reads a message, after waiting up to ``self.timeout`` seconds
//|
//|         If no message is received in time, `None` is returned.  Otherwise,
//|         a `Message` or `RemoteTransmissionRequest` is returned."""
//|         ...
static mp_obj_t canio_listener_receive(mp_obj_t self_in) {
    canio_listener_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_canio_listener_check_for_deinit(self);

    mp_obj_t message = common_hal_canio_listener_receive(self);
    // note: receive fills out the type field of the message

    if (message) {
        return message;
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(canio_listener_receive_obj, canio_listener_receive);

//|     def in_waiting(self) -> int:
//|         """Returns the number of messages (including remote
//|         transmission requests) waiting"""
//|         ...
static mp_obj_t canio_listener_in_waiting(mp_obj_t self_in) {
    canio_listener_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_canio_listener_check_for_deinit(self);
    return MP_OBJ_NEW_SMALL_INT(common_hal_canio_listener_in_waiting(self));
}
static MP_DEFINE_CONST_FUN_OBJ_1(canio_listener_in_waiting_obj, canio_listener_in_waiting);

//|     def __iter__(self) -> Listener:
//|         """Returns self
//|
//|         This method exists so that `Listener` can be used as an
//|         iterable"""
//|         ...
//|
//|     def __next__(self) -> Union[RemoteTransmissionRequest, Message]:
//|         """Reads a message, after waiting up to self.timeout seconds
//|
//|         If no message is received in time, raises StopIteration.  Otherwise,
//|         a Message or  is returned.
//|
//|         This method enables the `Listener` to be used as an
//|         iterable, for instance in a for-loop."""
//|         ...
static mp_obj_t canio_iternext(mp_obj_t self_in) {
    mp_obj_t result = canio_listener_receive(self_in);
    if (result == mp_const_none) {
        return MP_OBJ_STOP_ITERATION;
    }
    return result;
}

//|     def deinit(self) -> None:
//|         """Deinitialize this object, freeing its hardware resources"""
//|         ...
static mp_obj_t canio_listener_deinit(mp_obj_t self_in) {
    canio_listener_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_canio_listener_deinit(self);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(canio_listener_deinit_obj, canio_listener_deinit);

//|     def __enter__(self) -> CAN:
//|         """Returns self, to allow the object to be used in a `with` statement for resource control"""
//|         ...
static mp_obj_t canio_listener_enter(mp_obj_t self_in) {
    canio_listener_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_canio_listener_check_for_deinit(self);
    return self_in;
}
static MP_DEFINE_CONST_FUN_OBJ_1(canio_listener_enter_obj, canio_listener_enter);

//|     def __exit__(
//|         self,
//|         unused1: Optional[Type[BaseException]],
//|         unused2: Optional[BaseException],
//|         unused3: Optional[TracebackType],
//|     ) -> None:
//|         """Calls deinit()"""
//|         ...
static mp_obj_t canio_listener_exit(size_t num_args, const mp_obj_t args[]) {
    canio_listener_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    common_hal_canio_listener_deinit(self);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(canio_listener_exit_obj, 4, 4, canio_listener_exit);


//|     timeout: float
//|
static mp_obj_t canio_listener_timeout_get(mp_obj_t self_in) {
    canio_listener_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_canio_listener_check_for_deinit(self);
    return mp_obj_new_float(common_hal_canio_listener_get_timeout(self));
}
static MP_DEFINE_CONST_FUN_OBJ_1(canio_listener_timeout_get_obj, canio_listener_timeout_get);

static mp_obj_t canio_listener_timeout_set(mp_obj_t self_in, mp_obj_t timeout_in) {
    canio_listener_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_canio_listener_check_for_deinit(self);
    common_hal_canio_listener_set_timeout(self, mp_obj_get_float(timeout_in));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(canio_listener_timeout_set_obj, canio_listener_timeout_set);

MP_PROPERTY_GETSET(canio_listener_timeout_obj,
    (mp_obj_t)&canio_listener_timeout_get_obj,
    (mp_obj_t)&canio_listener_timeout_set_obj);



static const mp_rom_map_elem_t canio_listener_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&canio_listener_enter_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&canio_listener_exit_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&canio_listener_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_in_waiting), MP_ROM_PTR(&canio_listener_in_waiting_obj) },
    { MP_ROM_QSTR(MP_QSTR_receive), MP_ROM_PTR(&canio_listener_receive_obj) },
    { MP_ROM_QSTR(MP_QSTR_timeout), MP_ROM_PTR(&canio_listener_timeout_obj) },
};
static MP_DEFINE_CONST_DICT(canio_listener_locals_dict, canio_listener_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    canio_listener_type,
    MP_QSTR_Listener,
    MP_TYPE_FLAG_ITER_IS_ITERNEXT | MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    locals_dict, &canio_listener_locals_dict,
    iter, canio_iternext
    );
