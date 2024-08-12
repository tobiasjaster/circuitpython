// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Nick Moore for Adafruit Industries
//
// SPDX-License-Identifier: MIT


#include <math.h>
#include <string.h>

#include "py/obj.h"
#include "py/objproperty.h"
#include "py/runtime.h"

#include "common-hal/watchdog/WatchDogTimer.h"

#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/watchdog/__init__.h"
#include "shared-bindings/watchdog/WatchDogTimer.h"

#include "supervisor/port.h"

//| class WatchDogTimer:
//|     """Timer that is used to detect code lock ups and automatically reset the microcontroller
//|     when one is detected.
//|
//|     A lock up is detected when the watchdog hasn't been fed after a given duration. So, make
//|     sure to call `feed` within the timeout.
//|     """
//|

//|     def __init__(self) -> None:
//|         """Access the sole instance through `microcontroller.watchdog`."""
//|         ...

//|     def feed(self) -> None:
//|         """Feed the watchdog timer. This must be called regularly, otherwise
//|         the timer will expire. Silently does nothing if the watchdog isn't active."""
//|         ...
static mp_obj_t watchdog_watchdogtimer_feed(mp_obj_t self_in) {
    watchdog_watchdogtimer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (common_hal_watchdog_get_mode(self) != WATCHDOGMODE_NONE) {
        common_hal_watchdog_feed(self);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(watchdog_watchdogtimer_feed_obj, watchdog_watchdogtimer_feed);

//|     def deinit(self) -> None:
//|         """Stop the watchdog timer.
//|
//|         :raises RuntimeError: if the watchdog timer cannot be disabled on this platform.
//|
//|         .. note:: This is deprecated in ``9.0.0`` and will be removed in ``10.0.0``.
//|             Set watchdog `mode` to `None` instead.
//|
//|         """
//|         ...
static mp_obj_t watchdog_watchdogtimer_deinit(mp_obj_t self_in) {
    watchdog_watchdogtimer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_watchdog_deinit(self);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(watchdog_watchdogtimer_deinit_obj, watchdog_watchdogtimer_deinit);

//|     timeout: float
//|     """The maximum number of seconds that can elapse between calls
//|     to `feed()`. Setting the timeout will also feed the watchdog."""
static mp_obj_t watchdog_watchdogtimer_obj_get_timeout(mp_obj_t self_in) {
    watchdog_watchdogtimer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_float(common_hal_watchdog_get_timeout(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(watchdog_watchdogtimer_get_timeout_obj, watchdog_watchdogtimer_obj_get_timeout);

static mp_obj_t watchdog_watchdogtimer_obj_set_timeout(mp_obj_t self_in, mp_obj_t timeout_obj) {
    watchdog_watchdogtimer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_float_t timeout = mp_obj_get_float(timeout_obj);

    mp_arg_validate_int_min(timeout, 0, MP_QSTR_timeout);

    common_hal_watchdog_set_timeout(self, timeout);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(watchdog_watchdogtimer_set_timeout_obj, watchdog_watchdogtimer_obj_set_timeout);

MP_PROPERTY_GETSET(watchdog_watchdogtimer_timeout_obj,
    (mp_obj_t)&watchdog_watchdogtimer_get_timeout_obj,
    (mp_obj_t)&watchdog_watchdogtimer_set_timeout_obj);

//|     mode: Optional[WatchDogMode]
//|     """The current operating mode of the WatchDogTimer `watchdog.WatchDogMode` or `None` when
//|     the timer is disabled.
//|
//|     Setting a `WatchDogMode` activates the WatchDog::
//|
//|       from microcontroller import watchdog
//|       from watchdog import WatchDogMode
//|
//|       watchdog.timeout = 5
//|       watchdog.mode = WatchDogMode.RESET
//|
//|
//|     Once set, the `WatchDogTimer` will perform the specified action if the timer expires."""
//|
static mp_obj_t watchdog_watchdogtimer_obj_get_mode(mp_obj_t self_in) {
    watchdog_watchdogtimer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return cp_enum_find(&watchdog_watchdogmode_type, common_hal_watchdog_get_mode(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(watchdog_watchdogtimer_get_mode_obj, watchdog_watchdogtimer_obj_get_mode);

static mp_obj_t watchdog_watchdogtimer_obj_set_mode(mp_obj_t self_in, mp_obj_t obj) {
    watchdog_watchdogtimer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    watchdog_watchdogmode_t mode = (obj == mp_const_none) ? WATCHDOGMODE_NONE : cp_enum_value(&watchdog_watchdogmode_type, obj, MP_QSTR_mode);
    common_hal_watchdog_set_mode(self, mode);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(watchdog_watchdogtimer_set_mode_obj, watchdog_watchdogtimer_obj_set_mode);

MP_PROPERTY_GETSET(watchdog_watchdogtimer_mode_obj,
    (mp_obj_t)&watchdog_watchdogtimer_get_mode_obj,
    (mp_obj_t)&watchdog_watchdogtimer_set_mode_obj);

static const mp_rom_map_elem_t watchdog_watchdogtimer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_feed), MP_ROM_PTR(&watchdog_watchdogtimer_feed_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&watchdog_watchdogtimer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_timeout), MP_ROM_PTR(&watchdog_watchdogtimer_timeout_obj) },
    { MP_ROM_QSTR(MP_QSTR_mode), MP_ROM_PTR(&watchdog_watchdogtimer_mode_obj) },
};
static MP_DEFINE_CONST_DICT(watchdog_watchdogtimer_locals_dict, watchdog_watchdogtimer_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    watchdog_watchdogtimer_type,
    MP_QSTR_WatchDogTimer,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    locals_dict, &watchdog_watchdogtimer_locals_dict
    );
