// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Dan Halbert for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/board/__init__.h"
#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/alarm/pin/PinAlarm.h"

#include "py/nlr.h"
#include "py/obj.h"
#include "py/objproperty.h"
#include "py/runtime.h"

//| class PinAlarm:
//|     """Trigger an alarm when a pin changes state."""
//|
//|     def __init__(
//|         self, pin: microcontroller.Pin, value: bool, edge: bool = False, pull: bool = False
//|     ) -> None:
//|         """Create an alarm triggered by a `microcontroller.Pin` level. The alarm is not active
//|         until it is passed to an `alarm`-enabling function, such as `alarm.light_sleep_until_alarms()` or
//|         `alarm.exit_and_deep_sleep_until_alarms()`.
//|
//|         :param microcontroller.Pin pin: The pin to monitor. On some ports, the choice of pin
//|           may be limited due to hardware restrictions, particularly for deep-sleep alarms.
//|         :param bool value: When active, trigger when the pin value is high (``True``) or low (``False``).
//|           On some ports, multiple `PinAlarm` objects may need to have coordinated values
//|           for deep-sleep alarms.
//|         :param bool edge: If ``True``, trigger only when there is a transition to the specified
//|           value of ``value``. If ``True``, if the alarm becomes active when the pin value already
//|           matches ``value``, the alarm is not triggered: the pin must transition from ``not value``
//|           to ``value`` to trigger the alarm. On some ports, edge-triggering may not be available,
//|           particularly for deep-sleep alarms.
//|         :param bool pull: Enable a pull-up or pull-down which pulls the pin to the level opposite
//|           that of ``value``. For instance, if ``value`` is set to ``True``, setting ``pull``
//|           to ``True`` will enable a pull-down, to hold the pin low normally until an outside signal
//|           pulls it high.
//|         """
//|         ...
static mp_obj_t alarm_pin_pinalarm_make_new(const mp_obj_type_t *type, mp_uint_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    alarm_pin_pinalarm_obj_t *self = mp_obj_malloc(alarm_pin_pinalarm_obj_t, &alarm_pin_pinalarm_type);
    enum { ARG_pin, ARG_value, ARG_edge, ARG_pull };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_pin, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_value, MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_BOOL },
        { MP_QSTR_edge, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_pull, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    const mcu_pin_obj_t *pin = validate_obj_is_free_pin(args[ARG_pin].u_obj, MP_QSTR_pin);

    common_hal_alarm_pin_pinalarm_construct(self,
        pin,
        args[ARG_value].u_bool,
        args[ARG_edge].u_bool,
        args[ARG_pull].u_bool);

    return MP_OBJ_FROM_PTR(self);
}

//|     pin: microcontroller.Pin
//|     """The trigger pin."""
static mp_obj_t alarm_pin_pinalarm_obj_get_pin(mp_obj_t self_in) {
    alarm_pin_pinalarm_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const mcu_pin_obj_t *pin = common_hal_alarm_pin_pinalarm_get_pin(self);
    if (pin == NULL) {
        return mp_const_none;
    }
    return MP_OBJ_FROM_PTR(pin);
}
MP_DEFINE_CONST_FUN_OBJ_1(alarm_pin_pinalarm_get_pin_obj, alarm_pin_pinalarm_obj_get_pin);

MP_PROPERTY_GETTER(alarm_pin_pinalarm_pin_obj,
    (mp_obj_t)&alarm_pin_pinalarm_get_pin_obj);

//|     value: bool
//|     """The value on which to trigger."""
//|
static mp_obj_t alarm_pin_pinalarm_obj_get_value(mp_obj_t self_in) {
    alarm_pin_pinalarm_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(common_hal_alarm_pin_pinalarm_get_value(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(alarm_pin_pinalarm_get_value_obj, alarm_pin_pinalarm_obj_get_value);

MP_PROPERTY_GETTER(alarm_pin_pinalarm_value_obj,
    (mp_obj_t)&alarm_pin_pinalarm_get_value_obj);

static const mp_rom_map_elem_t alarm_pin_pinalarm_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_pin), MP_ROM_PTR(&alarm_pin_pinalarm_pin_obj) },
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&alarm_pin_pinalarm_value_obj) },
};

static MP_DEFINE_CONST_DICT(alarm_pin_pinalarm_locals_dict, alarm_pin_pinalarm_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    alarm_pin_pinalarm_type,
    MP_QSTR_PinAlarm,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    make_new, alarm_pin_pinalarm_make_new,
    locals_dict, &alarm_pin_pinalarm_locals_dict
    );
