// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2016 Scott Shawcroft
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/board/__init__.h"
#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/microcontroller/Pin.h"

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"

//| class Pin:
//|     """Identifies an IO pin on the microcontroller."""
//|
//|     def __init__(self) -> None:
//|         """Identifies an IO pin on the microcontroller. They are fixed by the
//|         hardware so they cannot be constructed on demand. Instead, use
//|         :mod:`board` or :mod:`microcontroller.pin` to reference the desired pin."""
//|         ...

//|     def __hash__(self) -> int:
//|         """Returns a hash for the Pin."""
//|         ...
//|
// Provided inherently.
// See https://github.com/micropython/micropython/pull/10348.


static void get_pin_name(const mcu_pin_obj_t *self, qstr *package, qstr *module, qstr *name) {
    const mp_map_t *board_map = &board_module_globals.map;
    for (uint8_t i = 0; i < board_map->alloc; i++) {
        if (board_map->table[i].value == MP_OBJ_FROM_PTR(self)) {
            *package = 0;
            *module = MP_QSTR_board;
            *name = MP_OBJ_QSTR_VALUE(board_map->table[i].key);
            return;
        }
    }
    const mp_map_t *mcu_map = &mcu_pin_globals.map;
    for (uint8_t i = 0; i < mcu_map->alloc; i++) {
        if (mcu_map->table[i].value == MP_OBJ_FROM_PTR(self)) {
            *package = MP_QSTR_microcontroller;
            *module = MP_QSTR_pin;
            *name = MP_OBJ_QSTR_VALUE(mcu_map->table[i].key);
            return;
        }
    }
}

void shared_bindings_microcontroller_pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mcu_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    qstr package = MP_QSTR_Pin;
    qstr module;
    qstr name = MP_QSTR_Pin;

    get_pin_name(self, &package, &module, &name);
    if (package) {
        mp_printf(print, "%q.%q.%q", package, module, name);
    } else {
        mp_printf(print, "%q.%q", module, name);
    }
}

MP_DEFINE_CONST_OBJ_TYPE(
    mcu_pin_type,
    MP_QSTR_Pin,
    MP_TYPE_FLAG_NONE,
    print, shared_bindings_microcontroller_pin_print
    );

const mcu_pin_obj_t *validate_obj_is_pin(mp_obj_t obj, qstr arg_name) {
    return MP_OBJ_TO_PTR(mp_arg_validate_type(obj, &mcu_pin_type, arg_name));
}

const mcu_pin_obj_t *validate_obj_is_pin_in(mp_obj_t obj, qstr arg_name) {
    return MP_OBJ_TO_PTR(mp_arg_validate_type_in(obj, &mcu_pin_type, arg_name));
}

// Validate that the obj is a pin or None. Return an mcu_pin_obj_t* or NULL, correspondingly.
const mcu_pin_obj_t *validate_obj_is_pin_or_none(mp_obj_t obj, qstr arg_name) {
    if (obj == mp_const_none) {
        return NULL;
    }
    return validate_obj_is_pin(obj, arg_name);
}

const mcu_pin_obj_t *validate_obj_is_free_pin(mp_obj_t obj, qstr arg_name) {
    const mcu_pin_obj_t *pin = validate_obj_is_pin(obj, arg_name);
    assert_pin_free(pin);
    return pin;
}

// Validate every element in the list is a unique pin
void validate_no_duplicate_pins(mp_obj_t seq, qstr arg_name) {
    const size_t num_pins = (size_t)MP_OBJ_SMALL_INT_VALUE(mp_obj_len(seq));

    for (size_t pin_cnt = 0; pin_cnt < num_pins; pin_cnt++) {
        mp_obj_t pin1_obj = mp_obj_subscr(seq, MP_OBJ_NEW_SMALL_INT(pin_cnt), MP_OBJ_SENTINEL);
        const mcu_pin_obj_t *pin1 = validate_obj_is_pin_in(pin1_obj, arg_name);

        for (size_t pin_cnt_2 = pin_cnt + 1; pin_cnt_2 < num_pins; pin_cnt_2++) {
            mp_obj_t pin2_obj = mp_obj_subscr(seq, MP_OBJ_NEW_SMALL_INT(pin_cnt_2), MP_OBJ_SENTINEL);
            const mcu_pin_obj_t *pin2 = validate_obj_is_pin_in(pin2_obj, arg_name);
            if (pin1 == pin2) {
                mp_raise_TypeError_varg(MP_ERROR_TEXT("%q contains duplicate pins"), arg_name);
            }
        }
    }
}

void validate_no_duplicate_pins_2(mp_obj_t seq1, mp_obj_t seq2, qstr arg_name1, qstr arg_name2) {
    const size_t num_pins_1 = (size_t)MP_OBJ_SMALL_INT_VALUE(mp_obj_len(seq1));
    const size_t num_pins_2 = (size_t)MP_OBJ_SMALL_INT_VALUE(mp_obj_len(seq2));

    validate_no_duplicate_pins(seq1, arg_name1);
    validate_no_duplicate_pins(seq2, arg_name2);

    for (size_t pin_cnt_1 = 0; pin_cnt_1 < num_pins_1; pin_cnt_1++) {
        mp_obj_t pin1_obj = mp_obj_subscr(seq1, MP_OBJ_NEW_SMALL_INT(pin_cnt_1), MP_OBJ_SENTINEL);
        const mcu_pin_obj_t *pin1 = validate_obj_is_pin_in(pin1_obj, arg_name1);

        for (size_t pin_cnt_2 = 0; pin_cnt_2 < num_pins_2; pin_cnt_2++) {
            mp_obj_t pin2_obj = mp_obj_subscr(seq2, MP_OBJ_NEW_SMALL_INT(pin_cnt_2), MP_OBJ_SENTINEL);
            const mcu_pin_obj_t *pin2 = validate_obj_is_pin_in(pin2_obj, arg_name2);
            if (pin1 == pin2) {
                mp_raise_TypeError_varg(MP_ERROR_TEXT("%q and %q contain duplicate pins"), arg_name1, arg_name2);
            }
        }
    }
}

// Validate every element in the list to be a free pin.
void validate_list_is_free_pins(qstr what, const mcu_pin_obj_t **pins_out, mp_int_t max_pins, mp_obj_t seq, uint8_t *count_out) {
    mp_int_t len = MP_OBJ_SMALL_INT_VALUE(mp_obj_len(seq));
    mp_arg_validate_length_max(len, max_pins, what);
    *count_out = len;
    for (mp_int_t i = 0; i < len; i++) {
        pins_out[i] =
            validate_obj_is_pin_in(mp_obj_subscr(seq, MP_OBJ_NEW_SMALL_INT(i), MP_OBJ_SENTINEL), what);
        assert_pin_free(pins_out[i]);
    }
}

// Validate that the obj is a free pin or None. Return an mcu_pin_obj_t* or NULL, correspondingly.
const mcu_pin_obj_t *validate_obj_is_free_pin_or_none(mp_obj_t obj, qstr arg_name) {
    if (obj == mp_const_none) {
        return NULL;
    }
    const mcu_pin_obj_t *pin = validate_obj_is_pin(obj, arg_name);
    assert_pin_free(pin);
    return pin;
}

void assert_pin_free(const mcu_pin_obj_t *pin) {
    if (pin != NULL && pin != MP_OBJ_TO_PTR(mp_const_none) && !common_hal_mcu_pin_is_free(pin)) {
        qstr package;
        qstr module;
        qstr name = MP_QSTR_Pin;

        get_pin_name(pin, &package, &module, &name);
        mp_raise_ValueError_varg(MP_ERROR_TEXT("%q in use"), name);
    }
}

void validate_pins(qstr what, uint8_t *pin_nos, mp_int_t max_pins, mp_obj_t seq, uint8_t *count_out) {
    const mcu_pin_obj_t *pins[max_pins];
    validate_list_is_free_pins(what, pins, max_pins, seq, count_out);
    for (mp_int_t i = 0; i < *count_out; i++) {
        pin_nos[i] = common_hal_mcu_pin_number(pins[i]);
    }
}

NORETURN void raise_ValueError_invalid_pin(void) {
    mp_arg_error_invalid(MP_QSTR_pin);
}

NORETURN void raise_ValueError_invalid_pins(void) {
    mp_arg_error_invalid(MP_QSTR_pins);
}

NORETURN void raise_ValueError_invalid_pin_name(qstr pin_name) {
    mp_raise_ValueError_varg(MP_ERROR_TEXT("Invalid %q pin"), pin_name);
}
