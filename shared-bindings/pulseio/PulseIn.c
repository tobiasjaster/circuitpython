// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "shared/runtime/context_manager_helpers.h"
#include "py/objproperty.h"
#include "py/runtime.h"
#include "py/runtime0.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/pulseio/PulseIn.h"
#include "shared-bindings/util.h"

//| class PulseIn:
//|     """Measure a series of active and idle pulses. This is commonly used in infrared receivers
//|     and low cost temperature sensors (DHT). The pulsed signal consists of timed active and
//|     idle periods. Unlike PWM, there is no set duration for active and idle pairs."""
//|
//|     def __init__(
//|         self, pin: microcontroller.Pin, maxlen: int = 2, *, idle_state: bool = False
//|     ) -> None:
//|         """Create a PulseIn object associated with the given pin. The object acts as
//|         a read-only sequence of pulse lengths with a given max length. When it is
//|         active, new pulse lengths are added to the end of the list. When there is
//|         no more room (len() == `maxlen`) the oldest pulse length is removed to
//|         make room.
//|
//|         :param ~microcontroller.Pin pin: Pin to read pulses from.
//|         :param int maxlen: Maximum number of pulse durations to store at once
//|         :param bool idle_state: Idle state of the pin. At start and after `resume`
//|           the first recorded pulse will the opposite state from idle.
//|
//|         Read a short series of pulses::
//|
//|           import pulseio
//|           import board
//|
//|           pulses = pulseio.PulseIn(board.D7)
//|
//|           # Wait for an active pulse
//|           while len(pulses) == 0:
//|               pass
//|           # Pause while we do something with the pulses
//|           pulses.pause()
//|
//|           # Print the pulses. pulses[0] is an active pulse unless the length
//|           # reached max length and idle pulses are recorded.
//|           print(pulses)
//|
//|           # Clear the rest
//|           pulses.clear()
//|
//|           # Resume with an 80 microsecond active pulse
//|           pulses.resume(80)"""
//|         ...
static mp_obj_t pulseio_pulsein_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_pin, ARG_maxlen, ARG_idle_state };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_pin, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_maxlen, MP_ARG_INT, {.u_int = 2} },
        { MP_QSTR_idle_state, MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    const mcu_pin_obj_t *pin = validate_obj_is_free_pin(args[ARG_pin].u_obj, MP_QSTR_pin);

    pulseio_pulsein_obj_t *self = m_new_obj_with_finaliser(pulseio_pulsein_obj_t);
    self->base.type = &pulseio_pulsein_type;

    common_hal_pulseio_pulsein_construct(self, pin, args[ARG_maxlen].u_int,
        args[ARG_idle_state].u_bool);

    return MP_OBJ_FROM_PTR(self);
}

//|     def deinit(self) -> None:
//|         """Deinitialises the PulseIn and releases any hardware resources for reuse."""
//|         ...
static mp_obj_t pulseio_pulsein_deinit(mp_obj_t self_in) {
    pulseio_pulsein_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_pulseio_pulsein_deinit(self);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pulseio_pulsein_deinit_obj, pulseio_pulsein_deinit);

static void check_for_deinit(pulseio_pulsein_obj_t *self) {
    if (common_hal_pulseio_pulsein_deinited(self)) {
        raise_deinited_error();
    }
}

//|     def __enter__(self) -> PulseIn:
//|         """No-op used by Context Managers."""
//|         ...
//  Provided by context manager helper.

//|     def __exit__(self) -> None:
//|         """Automatically deinitializes the hardware when exiting a context. See
//|         :ref:`lifetime-and-contextmanagers` for more info."""
//|         ...
static mp_obj_t pulseio_pulsein_obj___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    common_hal_pulseio_pulsein_deinit(args[0]);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pulseio_pulsein___exit___obj, 4, 4, pulseio_pulsein_obj___exit__);

//|     def pause(self) -> None:
//|         """Pause pulse capture"""
//|         ...
static mp_obj_t pulseio_pulsein_obj_pause(mp_obj_t self_in) {
    pulseio_pulsein_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);

    common_hal_pulseio_pulsein_pause(self);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pulseio_pulsein_pause_obj, pulseio_pulsein_obj_pause);

//|     def resume(self, trigger_duration: int = 0) -> None:
//|         """Resumes pulse capture after an optional trigger pulse.
//|
//|         .. warning:: Using trigger pulse with a device that drives both high and
//|           low signals risks a short. Make sure your device is open drain (only
//|           drives low) when using a trigger pulse. You most likely added a
//|           "pull-up" resistor to your circuit to do this.
//|
//|         :param int trigger_duration: trigger pulse duration in microseconds"""
//|         ...
static mp_obj_t pulseio_pulsein_obj_resume(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_trigger_duration };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_trigger_duration, MP_ARG_INT, {.u_int = 0} },
    };
    pulseio_pulsein_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    check_for_deinit(self);

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    common_hal_pulseio_pulsein_resume(self, args[ARG_trigger_duration].u_int);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(pulseio_pulsein_resume_obj, 1, pulseio_pulsein_obj_resume);

//|     def clear(self) -> None:
//|         """Clears all captured pulses"""
//|         ...
static mp_obj_t pulseio_pulsein_obj_clear(mp_obj_t self_in) {
    pulseio_pulsein_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);

    common_hal_pulseio_pulsein_clear(self);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pulseio_pulsein_clear_obj, pulseio_pulsein_obj_clear);

//|     def popleft(self) -> int:
//|         """Removes and returns the oldest read pulse duration in microseconds."""
//|         ...
static mp_obj_t pulseio_pulsein_obj_popleft(mp_obj_t self_in) {
    pulseio_pulsein_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);

    return MP_OBJ_NEW_SMALL_INT(common_hal_pulseio_pulsein_popleft(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(pulseio_pulsein_popleft_obj, pulseio_pulsein_obj_popleft);

//|     maxlen: int
//|     """The maximum length of the PulseIn. When len() is equal to maxlen,
//|     it is unclear which pulses are active and which are idle."""
static mp_obj_t pulseio_pulsein_obj_get_maxlen(mp_obj_t self_in) {
    pulseio_pulsein_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);

    return MP_OBJ_NEW_SMALL_INT(common_hal_pulseio_pulsein_get_maxlen(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(pulseio_pulsein_get_maxlen_obj, pulseio_pulsein_obj_get_maxlen);

MP_PROPERTY_GETTER(pulseio_pulsein_maxlen_obj,
    (mp_obj_t)&pulseio_pulsein_get_maxlen_obj);

//|     paused: bool
//|     """True when pulse capture is paused as a result of :py:func:`pause` or an error during capture
//|     such as a signal that is too fast."""
static mp_obj_t pulseio_pulsein_obj_get_paused(mp_obj_t self_in) {
    pulseio_pulsein_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);

    return mp_obj_new_bool(common_hal_pulseio_pulsein_get_paused(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(pulseio_pulsein_get_paused_obj, pulseio_pulsein_obj_get_paused);

MP_PROPERTY_GETTER(pulseio_pulsein_paused_obj,
    (mp_obj_t)&pulseio_pulsein_get_paused_obj);

//|     def __bool__(self) -> bool: ...
//|     def __len__(self) -> int:
//|         """Returns the number of pulse durations currently stored.
//|
//|         This allows you to::
//|
//|           pulses = pulseio.PulseIn(pin)
//|           print(len(pulses))"""
//|         ...
static mp_obj_t pulsein_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    pulseio_pulsein_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);
    uint16_t len = common_hal_pulseio_pulsein_get_len(self);
    switch (op) {
        case MP_UNARY_OP_BOOL:
            return mp_obj_new_bool(len != 0);
        case MP_UNARY_OP_LEN:
            return MP_OBJ_NEW_SMALL_INT(len);
        default:
            return MP_OBJ_NULL;      // op not supported
    }
}

//|     def __getitem__(self, index: int) -> Optional[int]:
//|         """Returns the value at the given index or values in slice.
//|
//|         This allows you to::
//|
//|           pulses = pulseio.PulseIn(pin)
//|           print(pulses[0])"""
//|         ...
//|
static mp_obj_t pulsein_subscr(mp_obj_t self_in, mp_obj_t index_obj, mp_obj_t value) {
    if (value == mp_const_none) {
        // delete item
        mp_raise_AttributeError(MP_ERROR_TEXT("Cannot delete values"));
    } else {
        pulseio_pulsein_obj_t *self = MP_OBJ_TO_PTR(self_in);
        check_for_deinit(self);

        if (mp_obj_is_type(index_obj, &mp_type_slice)) {
            mp_raise_NotImplementedError(MP_ERROR_TEXT("Slices not supported"));
        } else {
            size_t index = mp_get_index(&pulseio_pulsein_type, common_hal_pulseio_pulsein_get_len(self), index_obj, false);
            if (value == MP_OBJ_SENTINEL) {
                // load
                return MP_OBJ_NEW_SMALL_INT(common_hal_pulseio_pulsein_get_item(self, index));
            } else {
                mp_raise_AttributeError(MP_ERROR_TEXT("Read-only"));
            }
        }
    }
    return mp_const_none;
}

static const mp_rom_map_elem_t pulseio_pulsein_locals_dict_table[] = {
    // Methods
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pulseio_pulsein_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&pulseio_pulsein_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&default___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&pulseio_pulsein___exit___obj) },
    { MP_ROM_QSTR(MP_QSTR_pause), MP_ROM_PTR(&pulseio_pulsein_pause_obj) },
    { MP_ROM_QSTR(MP_QSTR_resume), MP_ROM_PTR(&pulseio_pulsein_resume_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&pulseio_pulsein_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_popleft), MP_ROM_PTR(&pulseio_pulsein_popleft_obj) },

    // Properties
    { MP_ROM_QSTR(MP_QSTR_maxlen), MP_ROM_PTR(&pulseio_pulsein_maxlen_obj) },
    { MP_ROM_QSTR(MP_QSTR_paused), MP_ROM_PTR(&pulseio_pulsein_paused_obj) },
};
static MP_DEFINE_CONST_DICT(pulseio_pulsein_locals_dict, pulseio_pulsein_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    pulseio_pulsein_type,
    MP_QSTR_PulseIn,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    make_new, pulseio_pulsein_make_new,
    locals_dict, &pulseio_pulsein_locals_dict,
    subscr, pulsein_subscr,
    unary_op, pulsein_unary_op
    );
