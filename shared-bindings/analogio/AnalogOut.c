// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2013, 2014 Damien P. George
//
// SPDX-License-Identifier: MIT

#include <stdint.h>
#include <string.h>

#include "shared/runtime/context_manager_helpers.h"
#include "py/objproperty.h"
#include "py/runtime.h"

#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/analogio/AnalogOut.h"
#include "shared-bindings/util.h"

//| class AnalogOut:
//|     """Output analog values (a specific voltage).
//|
//|     **Limitations:** Not available on Nordic, RP2040, Spresense, as there is no on-chip DAC.
//|     On Espressif, available only on ESP32 and ESP32-S2; other chips do not have a DAC.
//|
//|     Example usage::
//|
//|         import analogio
//|         from board import *
//|
//|         dac = analogio.AnalogOut(A2)                # output on pin A2
//|         dac.value = 32768                           # makes A2 1.65V"""
//|
//|     def __init__(self, pin: microcontroller.Pin) -> None:
//|         """Use the AnalogOut on the given pin.
//|
//|         :param ~microcontroller.Pin pin: the pin to output to
//|
//|         """
//|         ...
static mp_obj_t analogio_analogout_make_new(const mp_obj_type_t *type, mp_uint_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, 1, false);

    const mcu_pin_obj_t *pin = validate_obj_is_free_pin(args[0], MP_QSTR_pin);

    analogio_analogout_obj_t *self = mp_obj_malloc(analogio_analogout_obj_t, &analogio_analogout_type);
    common_hal_analogio_analogout_construct(self, pin);

    return MP_OBJ_FROM_PTR(self);
}

//|     def deinit(self) -> None:
//|         """Turn off the AnalogOut and release the pin for other use."""
//|         ...
static mp_obj_t analogio_analogout_deinit(mp_obj_t self_in) {
    analogio_analogout_obj_t *self = self_in;

    common_hal_analogio_analogout_deinit(self);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(analogio_analogout_deinit_obj, analogio_analogout_deinit);

//|     def __enter__(self) -> AnalogOut:
//|         """No-op used by Context Managers."""
//|         ...
//  Provided by context manager helper.

//|     def __exit__(self) -> None:
//|         """Automatically deinitializes the hardware when exiting a context. See
//|         :ref:`lifetime-and-contextmanagers` for more info."""
//|         ...
static mp_obj_t analogio_analogout___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    common_hal_analogio_analogout_deinit(args[0]);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(analogio_analogout___exit___obj, 4, 4, analogio_analogout___exit__);

//|     value: int
//|     """The value on the analog pin between 0 and 65535 inclusive (16-bit). (write-only)
//|
//|     Even if the underlying digital to analog converter (DAC) is lower
//|     resolution, the value is 16-bit."""
//|
static mp_obj_t analogio_analogout_obj_set_value(mp_obj_t self_in, mp_obj_t value) {
    analogio_analogout_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (common_hal_analogio_analogout_deinited(self)) {
        raise_deinited_error();
    }
    uint16_t v = mp_arg_validate_int_range(mp_obj_get_int(value), 0, 65535, MP_QSTR_value);

    common_hal_analogio_analogout_set_value(self, v);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(analogio_analogout_set_value_obj, analogio_analogout_obj_set_value);

MP_PROPERTY_GETSET(analogio_analogout_value_obj,
    MP_ROM_NONE,
    (mp_obj_t)&analogio_analogout_set_value_obj);

static const mp_rom_map_elem_t analogio_analogout_locals_dict_table[] = {
    // instance methods
    { MP_OBJ_NEW_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&analogio_analogout_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__),  MP_ROM_PTR(&default___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__),   MP_ROM_PTR(&analogio_analogout___exit___obj) },

    // Properties
    { MP_OBJ_NEW_QSTR(MP_QSTR_value), (mp_obj_t)&analogio_analogout_value_obj },
};

static MP_DEFINE_CONST_DICT(analogio_analogout_locals_dict, analogio_analogout_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    analogio_analogout_type,
    MP_QSTR_AnalogOut,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    make_new, analogio_analogout_make_new,
    locals_dict, &analogio_analogout_locals_dict
    );
