// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2024 CDarius
//
// SPDX-License-Identifier: MIT

#include "shared/runtime/context_manager_helpers.h"
#include "py/binary.h"
#include "py/objproperty.h"
#include "py/runtime.h"
#include "shared-bindings/keypad/__init__.h"
#include "shared-bindings/keypad/Event.h"
#include "shared-bindings/keypad_demux/DemuxKeyMatrix.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/util.h"

//| class DemuxKeyMatrix:
//|     """Manage Cardputer 2D matrix of keys with a demultiplexer to drive rows and pins on columns.
//|
//|     .. raw:: html
//|
//|         <p>
//|         <details>
//|         <summary>Available on these boards</summary>
//|         <ul>
//|         {% for board in support_matrix_reverse["keypad_demux.DemuxKeyMatrix"] %}
//|         <li> {{ board }}
//|         {% endfor %}
//|         </ul>
//|         </details>
//|         </p>
//|
//|     """
//|
//|     def __init__(
//|         self,
//|         row_addr_pins: Sequence[microcontroller.Pin],
//|         column_pins: Sequence[microcontroller.Pin],
//|         interval: float = 0.020,
//|         max_events: int = 64,
//|         debounce_threshold: int = 1,
//|     ) -> None:
//|         """
//|         Create a `keypad.Keys` object that will scan the key matrix attached to the given row and column pins.
//|         There should not be any external pull-ups or pull-downs on the matrix:
//|         ``DemuxKeyMatrix`` enables internal pull-ups or pull-downs on the pins as necessary.
//|
//|         The keys are numbered sequentially from zero. A key number can be computed
//|         by ``row * len(column_pins) + column``.
//|
//|         An `keypad.EventQueue` is created when this object is created and is available in the `events` attribute.
//|
//|         :param Sequence[microcontroller.Pin] row_addr_pins: The pins attached to the rows demultiplexer.
//|         :param Sequence[microcontroller.Pin] column_pins: The pins attached to the columns.
//|         :param float interval: Scan keys no more often than ``interval`` to allow for debouncing.
//|           ``interval`` is in float seconds. The default is 0.020 (20 msecs).
//|         :param int max_events: maximum size of `events` `keypad.EventQueue`:
//|           maximum number of key transition events that are saved.
//|           Must be >= 1.
//|           If a new event arrives when the queue is full, the oldest event is discarded.
//|         :param int debounce_threshold: Emit events for state changes only after a key has been
//|           in the respective state for ``debounce_threshold`` times on average.
//|           Successive measurements are spaced apart by ``interval`` seconds.
//|           The default is 1, which resolves immediately. The maximum is 127.
//|         """
//|         ...

static mp_obj_t keypad_demux_demuxkeymatrix_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    keypad_demux_demuxkeymatrix_obj_t *self = mp_obj_malloc(keypad_demux_demuxkeymatrix_obj_t, &keypad_demux_demuxkeymatrix_type);
    enum { ARG_row_addr_pins, ARG_column_pins, ARG_interval, ARG_max_events, ARG_debounce_threshold };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_row_addr_pins, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_column_pins, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_interval, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_max_events, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 64} },
        { MP_QSTR_debounce_threshold, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_t row_addr_pins = args[ARG_row_addr_pins].u_obj;
    // mp_obj_len() will be >= 0.
    const size_t num_row_addr_pins = (size_t)MP_OBJ_SMALL_INT_VALUE(mp_obj_len(row_addr_pins));

    mp_obj_t column_pins = args[ARG_column_pins].u_obj;
    const size_t num_column_pins = (size_t)MP_OBJ_SMALL_INT_VALUE(mp_obj_len(column_pins));

    const mp_float_t interval =
        mp_arg_validate_obj_float_non_negative(args[ARG_interval].u_obj, 0.020f, MP_QSTR_interval);
    const size_t max_events = (size_t)mp_arg_validate_int_min(args[ARG_max_events].u_int, 1, MP_QSTR_max_events);
    const uint8_t debounce_threshold = (uint8_t)mp_arg_validate_int_range(args[ARG_debounce_threshold].u_int, 1, 127, MP_QSTR_debounce_threshold);

    const mcu_pin_obj_t *row_addr_pins_array[num_row_addr_pins];
    const mcu_pin_obj_t *column_pins_array[num_column_pins];

    validate_no_duplicate_pins_2(row_addr_pins, column_pins, MP_QSTR_row_addr_pins, MP_QSTR_column_pins);

    for (size_t row_addr = 0; row_addr < num_row_addr_pins; row_addr++) {
        const mcu_pin_obj_t *pin =
            validate_obj_is_free_pin(mp_obj_subscr(row_addr_pins, MP_OBJ_NEW_SMALL_INT(row_addr), MP_OBJ_SENTINEL), MP_QSTR_pin);
        row_addr_pins_array[row_addr] = pin;
    }

    for (size_t column = 0; column < num_column_pins; column++) {
        const mcu_pin_obj_t *pin =
            validate_obj_is_free_pin(mp_obj_subscr(column_pins, MP_OBJ_NEW_SMALL_INT(column), MP_OBJ_SENTINEL), MP_QSTR_pin);
        column_pins_array[column] = pin;
    }

    common_hal_keypad_demux_demuxkeymatrix_construct(self, num_row_addr_pins, row_addr_pins_array, num_column_pins, column_pins_array, interval, max_events, debounce_threshold);
    return MP_OBJ_FROM_PTR(self);
}

//|     def deinit(self) -> None:
//|         """Stop scanning and release the pins."""
//|         ...
static mp_obj_t keypad_demux_demuxkeymatrix_deinit(mp_obj_t self_in) {
    keypad_demux_demuxkeymatrix_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_keypad_demux_demuxkeymatrix_deinit(self);
    return MP_ROM_NONE;
}
MP_DEFINE_CONST_FUN_OBJ_1(keypad_demux_demuxkeymatrix_deinit_obj, keypad_demux_demuxkeymatrix_deinit);

//|     def __enter__(self) -> DemuxKeyMatrix:
//|         """No-op used by Context Managers."""
//|         ...
//  Provided by context manager helper.

//|     def __exit__(self) -> None:
//|         """Automatically deinitializes when exiting a context. See
//|         :ref:`lifetime-and-contextmanagers` for more info."""
//|         ...
static mp_obj_t keypad_demux_demuxkeymatrix___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    common_hal_keypad_demux_demuxkeymatrix_deinit(args[0]);
    return MP_ROM_NONE;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(keypad_demux_demuxkeymatrix___exit___obj, 4, 4, keypad_demux_demuxkeymatrix___exit__);

static void check_for_deinit(keypad_demux_demuxkeymatrix_obj_t *self) {
    if (common_hal_keypad_deinited(self)) {
        raise_deinited_error();
    }
}

//|     def reset(self) -> None:
//|         """Reset the internal state of the scanner to assume that all keys are now released.
//|         Any key that is already pressed at the time of this call will therefore immediately cause
//|         a new key-pressed event to occur.
//|         """
//|         ...

//|     key_count: int
//|     """The number of keys that are being scanned. (read-only)
//|     """

//|     def key_number_to_row_column(self, key_number: int) -> Tuple[int]:
//|         """Return the row and column for the given key number.
//|         The row is ``key_number // len(column_pins)``.
//|         The column is ``key_number % len(column_pins)``.
//|
//|         :return: ``(row, column)``
//|         :rtype: Tuple[int]
//|         """
//|         ...
static mp_obj_t keypad_demux_demuxkeymatrix_key_number_to_row_column(mp_obj_t self_in, mp_obj_t key_number_in) {
    keypad_demux_demuxkeymatrix_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);

    const mp_uint_t key_number = (mp_uint_t)mp_arg_validate_int_range(
        mp_obj_get_int(key_number_in),
        0, (mp_int_t)common_hal_keypad_generic_get_key_count(self),
        MP_QSTR_key_number);

    mp_uint_t row;
    mp_uint_t column;
    common_hal_keypad_demux_demuxkeymatrix_key_number_to_row_column(self, key_number, &row, &column);

    mp_obj_t row_column[2];
    row_column[0] = MP_OBJ_NEW_SMALL_INT(row);
    row_column[1] = MP_OBJ_NEW_SMALL_INT(column);

    return mp_obj_new_tuple(2, row_column);
}
MP_DEFINE_CONST_FUN_OBJ_2(keypad_demux_demuxkeymatrix_key_number_to_row_column_obj, keypad_demux_demuxkeymatrix_key_number_to_row_column);

//|     def row_column_to_key_number(self, row: int, column: int) -> int:
//|         """Return the key number for a given row and column.
//|         The key number is ``row * len(column_pins) + column``.
//|         """
//|         ...
static mp_obj_t keypad_demux_demuxkeymatrix_row_column_to_key_number(mp_obj_t self_in, mp_obj_t row_in, mp_obj_t column_in) {
    keypad_demux_demuxkeymatrix_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);

    const mp_uint_t row = (mp_uint_t)mp_arg_validate_int_range(
        mp_obj_get_int(row_in), 0, (mp_int_t)common_hal_keypad_demux_demuxkeymatrix_get_row_count(self), MP_QSTR_row);

    const mp_int_t column = (mp_uint_t)mp_arg_validate_int_range(
        mp_obj_get_int(column_in), 0, (mp_int_t)common_hal_keypad_demux_demuxkeymatrix_get_column_count(self), MP_QSTR_column);

    return MP_OBJ_NEW_SMALL_INT(
        (mp_int_t)common_hal_keypad_demux_demuxkeymatrix_row_column_to_key_number(self, row, column));
}
MP_DEFINE_CONST_FUN_OBJ_3(keypad_demux_demuxkeymatrix_row_column_to_key_number_obj, keypad_demux_demuxkeymatrix_row_column_to_key_number);

//|     events: keypad.EventQueue
//|     """The `keypad.EventQueue` associated with this `keypad.Keys` object. (read-only)
//|     """
//|

static const mp_rom_map_elem_t keypad_demux_demuxkeymatrix_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_deinit),                   MP_ROM_PTR(&keypad_demux_demuxkeymatrix_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__),                MP_ROM_PTR(&default___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__),                 MP_ROM_PTR(&keypad_demux_demuxkeymatrix___exit___obj) },

    { MP_ROM_QSTR(MP_QSTR_events),                   MP_ROM_PTR(&keypad_generic_events_obj) },
    { MP_ROM_QSTR(MP_QSTR_key_count),                MP_ROM_PTR(&keypad_generic_key_count_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset),                    MP_ROM_PTR(&keypad_generic_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_key_number_to_row_column), MP_ROM_PTR(&keypad_demux_demuxkeymatrix_key_number_to_row_column_obj) },
    { MP_ROM_QSTR(MP_QSTR_row_column_to_key_number), MP_ROM_PTR(&keypad_demux_demuxkeymatrix_row_column_to_key_number_obj) },
};

static MP_DEFINE_CONST_DICT(keypad_demux_demuxkeymatrix_locals_dict, keypad_demux_demuxkeymatrix_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    keypad_demux_demuxkeymatrix_type,
    MP_QSTR_DemuxKeyMatrix,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    make_new, keypad_demux_demuxkeymatrix_make_new,
    locals_dict, &keypad_demux_demuxkeymatrix_locals_dict
    );
