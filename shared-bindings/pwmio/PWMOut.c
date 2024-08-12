// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2016 Damien P. George
//
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "shared/runtime/context_manager_helpers.h"
#include "py/objproperty.h"
#include "py/runtime.h"

#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/pwmio/PWMOut.h"
#include "shared-bindings/util.h"


void common_hal_pwmio_pwmout_raise_error(pwmout_result_t result) {
    switch (result) {
        case PWMOUT_OK:
            break;
        case PWMOUT_INVALID_PIN:
            raise_ValueError_invalid_pin();
            break;
        case PWMOUT_INVALID_FREQUENCY:
            mp_arg_error_invalid(MP_QSTR_frequency);
            break;
        case PWMOUT_INVALID_FREQUENCY_ON_PIN:
            mp_arg_error_invalid(MP_QSTR_frequency);
            break;
        case PWMOUT_VARIABLE_FREQUENCY_NOT_AVAILABLE:
            mp_arg_error_invalid(MP_QSTR_variable_frequency);
            break;
        case PWMOUT_INTERNAL_RESOURCES_IN_USE:
            mp_raise_RuntimeError(MP_ERROR_TEXT("Internal resource(s) in use"));
            break;
        default:
        case PWMOUT_INITIALIZATION_ERROR:
            mp_raise_RuntimeError(MP_ERROR_TEXT("Internal error"));
            break;
    }
}

//| class PWMOut:
//|     """Output a Pulse Width Modulated signal on a given pin.
//|
//|     .. note:: The exact frequencies possible depend on the specific microcontroller.
//|       If the requested frequency is within the available range, one of the two
//|       nearest possible frequencies to the requested one is selected.
//|
//|       If the requested frequency is outside the range, either (A) a ValueError
//|       may be raised or (B) the highest or lowest frequency is selected. This
//|       behavior is microcontroller-dependent, and may depend on whether it's the
//|       upper or lower bound that is exceeded.
//|
//|       In any case, the actual frequency (rounded to 1Hz) is available in the
//|       ``frequency`` property after construction.
//|
//|     .. note:: The frequency is calculated based on a nominal CPU frequency.
//|       However, depending on the board, the error between the nominal and
//|       actual CPU frequency can be large (several hundred PPM in the case of
//|       crystal oscillators and up to ten percent in the case of RC
//|       oscillators)
//|
//|     """
//|
//|     def __init__(
//|         self,
//|         pin: microcontroller.Pin,
//|         *,
//|         duty_cycle: int = 0,
//|         frequency: int = 500,
//|         variable_frequency: bool = False
//|     ) -> None:
//|         """Create a PWM object associated with the given pin. This allows you to
//|         write PWM signals out on the given pin. Frequency is fixed after init
//|         unless ``variable_frequency`` is True.
//|
//|         .. note:: When ``variable_frequency`` is True, further PWM outputs may be
//|           limited because it may take more internal resources to be flexible. So,
//|           when outputting both fixed and flexible frequency signals construct the
//|           fixed outputs first.
//|
//|         :param ~microcontroller.Pin pin: The pin to output to
//|         :param int duty_cycle: The fraction of each pulse which is high. 16-bit
//|         :param int frequency: The target frequency in Hertz (32-bit)
//|         :param bool variable_frequency: True if the frequency will change over time
//|
//|
//|         Simple LED on::
//|
//|           import pwmio
//|           import board
//|
//|           pwm = pwmio.PWMOut(board.LED)
//|
//|           while True:
//|               pwm.duty_cycle = 2 ** 15  # Cycles the pin with 50% duty cycle (half of 2 ** 16) at the default 500hz
//|
//|         PWM LED fade::
//|
//|           import pwmio
//|           import board
//|
//|           pwm = pwmio.PWMOut(board.LED)  # output on LED pin with default of 500Hz
//|
//|           while True:
//|               for cycle in range(0, 65535):  # Cycles through the full PWM range from 0 to 65535
//|                   pwm.duty_cycle = cycle  # Cycles the LED pin duty cycle through the range of values
//|               for cycle in range(65534, 0, -1):  # Cycles through the PWM range backwards from 65534 to 0
//|                   pwm.duty_cycle = cycle  # Cycles the LED pin duty cycle through the range of values
//|
//|         PWM at specific frequency (servos and motors)::
//|
//|           import pwmio
//|           import board
//|
//|           pwm = pwmio.PWMOut(board.D13, frequency=50)
//|           pwm.duty_cycle = 2 ** 15  # Cycles the pin with 50% duty cycle (half of 2 ** 16) at 50hz
//|
//|         Variable frequency (usually tones)::
//|
//|           import pwmio
//|           import board
//|           import time
//|
//|           pwm = pwmio.PWMOut(board.D13, duty_cycle=2 ** 15, frequency=440, variable_frequency=True)
//|           time.sleep(0.2)
//|           pwm.frequency = 880
//|           time.sleep(0.1)
//|
//|         """
//|         ...
static mp_obj_t pwmio_pwmout_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    enum { ARG_pin, ARG_duty_cycle, ARG_frequency, ARG_variable_frequency };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_pin, MP_ARG_REQUIRED | MP_ARG_OBJ, },
        { MP_QSTR_duty_cycle, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_frequency, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 500} },
        { MP_QSTR_variable_frequency, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t parsed_args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed_args);

    const mcu_pin_obj_t *pin = validate_obj_is_free_pin(parsed_args[ARG_pin].u_obj, MP_QSTR_pin);

    uint16_t duty_cycle = parsed_args[ARG_duty_cycle].u_int;
    uint32_t frequency = parsed_args[ARG_frequency].u_int;
    bool variable_frequency = parsed_args[ARG_variable_frequency].u_bool;

    // create PWM object from the given pin
    pwmio_pwmout_obj_t *self = m_new_obj_with_finaliser(pwmio_pwmout_obj_t);
    self->base.type = &pwmio_pwmout_type;
    pwmout_result_t result = common_hal_pwmio_pwmout_construct(self, pin, duty_cycle, frequency, variable_frequency);
    common_hal_pwmio_pwmout_raise_error(result);

    return MP_OBJ_FROM_PTR(self);
}

//|     def deinit(self) -> None:
//|         """Deinitialises the PWMOut and releases any hardware resources for reuse."""
//|         ...
static mp_obj_t pwmio_pwmout_deinit(mp_obj_t self_in) {
    pwmio_pwmout_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_pwmio_pwmout_deinit(self);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pwmio_pwmout_deinit_obj, pwmio_pwmout_deinit);

static void check_for_deinit(pwmio_pwmout_obj_t *self) {
    if (common_hal_pwmio_pwmout_deinited(self)) {
        raise_deinited_error();
    }
}

//|     def __enter__(self) -> PWMOut:
//|         """No-op used by Context Managers."""
//|         ...
//  Provided by context manager helper.

//|     def __exit__(self) -> None:
//|         """Automatically deinitializes the hardware when exiting a context. See
//|         :ref:`lifetime-and-contextmanagers` for more info."""
//|         ...
static mp_obj_t pwmio_pwmout_obj___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    common_hal_pwmio_pwmout_deinit(args[0]);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pwmio_pwmout___exit___obj, 4, 4, pwmio_pwmout_obj___exit__);

//|     duty_cycle: int
//|     """16 bit value that dictates how much of one cycle is high (1) versus low
//|     (0). 0xffff will always be high, 0 will always be low and 0x7fff will
//|     be half high and then half low.
//|
//|     Depending on how PWM is implemented on a specific board, the internal
//|     representation for duty cycle might have less than 16 bits of resolution.
//|     Reading this property will return the value from the internal representation,
//|     so it may differ from the value set."""
static mp_obj_t pwmio_pwmout_obj_get_duty_cycle(mp_obj_t self_in) {
    pwmio_pwmout_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);
    return MP_OBJ_NEW_SMALL_INT(common_hal_pwmio_pwmout_get_duty_cycle(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(pwmio_pwmout_get_duty_cycle_obj, pwmio_pwmout_obj_get_duty_cycle);

static mp_obj_t pwmio_pwmout_obj_set_duty_cycle(mp_obj_t self_in, mp_obj_t duty_cycle) {
    pwmio_pwmout_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);
    mp_int_t duty = mp_obj_get_int(duty_cycle);

    mp_arg_validate_int_range(duty, 0, 0xffff, MP_QSTR_duty_cycle);

    common_hal_pwmio_pwmout_set_duty_cycle(self, duty);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(pwmio_pwmout_set_duty_cycle_obj, pwmio_pwmout_obj_set_duty_cycle);

MP_PROPERTY_GETSET(pwmio_pwmout_duty_cycle_obj,
    (mp_obj_t)&pwmio_pwmout_get_duty_cycle_obj,
    (mp_obj_t)&pwmio_pwmout_set_duty_cycle_obj);

//|     frequency: int
//|     """32 bit value that dictates the PWM frequency in Hertz (cycles per
//|     second). Only writeable when constructed with ``variable_frequency=True``.
//|
//|     Depending on how PWM is implemented on a specific board, the internal value
//|     for the PWM's duty cycle may need to be recalculated when the frequency
//|     changes. In these cases, the duty cycle is automatically recalculated
//|     from the original duty cycle value. This should happen without any need
//|     to manually re-set the duty cycle. However, an output glitch may occur during the adjustment.
//|     """
//|
static mp_obj_t pwmio_pwmout_obj_get_frequency(mp_obj_t self_in) {
    pwmio_pwmout_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);
    return MP_OBJ_NEW_SMALL_INT(common_hal_pwmio_pwmout_get_frequency(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(pwmio_pwmout_get_frequency_obj, pwmio_pwmout_obj_get_frequency);

static mp_obj_t pwmio_pwmout_obj_set_frequency(mp_obj_t self_in, mp_obj_t frequency) {
    pwmio_pwmout_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);
    if (!common_hal_pwmio_pwmout_get_variable_frequency(self)) {
        mp_raise_msg_varg(&mp_type_AttributeError, MP_ERROR_TEXT("Invalid %q"), MP_QSTR_variable_frequency);
    }
    mp_int_t freq = mp_obj_get_int(frequency);
    if (freq == 0) {
        mp_arg_error_invalid(MP_QSTR_frequency);
    }
    common_hal_pwmio_pwmout_set_frequency(self, freq);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(pwmio_pwmout_set_frequency_obj, pwmio_pwmout_obj_set_frequency);

MP_PROPERTY_GETSET(pwmio_pwmout_frequency_obj,
    (mp_obj_t)&pwmio_pwmout_get_frequency_obj,
    (mp_obj_t)&pwmio_pwmout_set_frequency_obj);

static const mp_rom_map_elem_t pwmio_pwmout_locals_dict_table[] = {
    // Methods
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pwmio_pwmout_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&pwmio_pwmout_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&default___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&pwmio_pwmout___exit___obj) },

    // Properties
    { MP_ROM_QSTR(MP_QSTR_duty_cycle), MP_ROM_PTR(&pwmio_pwmout_duty_cycle_obj) },
    { MP_ROM_QSTR(MP_QSTR_frequency), MP_ROM_PTR(&pwmio_pwmout_frequency_obj) },
    // TODO(tannewt): Add enabled to determine whether the signal is output
    // without giving up the resources. Useful for IR output.
};
static MP_DEFINE_CONST_DICT(pwmio_pwmout_locals_dict, pwmio_pwmout_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    pwmio_pwmout_type,
    MP_QSTR_PWMOut,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    make_new, pwmio_pwmout_make_new,
    locals_dict, &pwmio_pwmout_locals_dict
    );
