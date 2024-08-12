// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "py/obj.h"
#include "py/runtime.h"

#include "bindings/rp2pio/StateMachine.h"
#include "bindings/rp2pio/__init__.h"

//| """Hardware interface to RP2 series' programmable IO (PIO) peripheral.
//|
//| .. note:: This module is intended to be used with the `adafruit_pioasm library
//|     <https://github.com/adafruit/Adafruit_CircuitPython_PIOASM>`_.  For an
//|     introduction and guide to working with PIO in CircuitPython, see `this
//|     Learn guide <https://learn.adafruit.com/intro-to-rp2040-pio-with-circuitpython>`_.
//|
//| """
//|

//| def pins_are_sequential(pins: List[microcontroller.Pin]) -> bool:
//|     """Return True if the pins have sequential GPIO numbers, False otherwise"""
//|     ...
//|
static mp_obj_t rp2pio_pins_are_sequential(mp_obj_t pins_obj) {
    size_t len;
    mp_obj_t *items;
    mp_obj_get_array(pins_obj, &len, &items);

    const mcu_pin_obj_t *pins[len];
    for (size_t i = 0; i < len; i++) {
        pins[i] = validate_obj_is_pin(items[i], MP_QSTR_pins);
    }

    return mp_obj_new_bool(common_hal_rp2pio_pins_are_sequential(len, pins));
}

static MP_DEFINE_CONST_FUN_OBJ_1(rp2pio_pins_are_sequential_obj, rp2pio_pins_are_sequential);

static const mp_rom_map_elem_t rp2pio_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_rp2pio) },
    { MP_ROM_QSTR(MP_QSTR_StateMachine),  MP_ROM_PTR(&rp2pio_statemachine_type) },
    { MP_ROM_QSTR(MP_QSTR_pins_are_sequential),  MP_ROM_PTR(&rp2pio_pins_are_sequential_obj) },
};

static MP_DEFINE_CONST_DICT(rp2pio_module_globals, rp2pio_module_globals_table);

const mp_obj_module_t rp2pio_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&rp2pio_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_rp2pio, rp2pio_module);
