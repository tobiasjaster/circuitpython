// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/microcontroller/RunMode.h"

//| class RunMode:
//|     """run state of the microcontroller"""
//|
//|     def __init__(self) -> None:
//|         """Enum-like class to define the run mode of the microcontroller and
//|         CircuitPython."""
//|     NORMAL: RunMode
//|     """Run CircuitPython as normal.
//|
//|     :type microcontroller.RunMode:"""
//|
//|     SAFE_MODE: RunMode
//|     """Run CircuitPython in safe mode. User code will not run and the
//|     file system will be writeable over USB.
//|
//|     :type microcontroller.RunMode:"""
//|
//|     UF2: RunMode
//|     """Run the uf2 bootloader.
//|
//|     :type microcontroller.RunMode:"""
//|
//|     BOOTLOADER: RunMode
//|     """Run the default bootloader.
//|
//|     :type microcontroller.RunMode:"""
//|
const mp_obj_type_t mcu_runmode_type;

const mcu_runmode_obj_t mcu_runmode_uf2_obj = {
    { &mcu_runmode_type },
};

const mcu_runmode_obj_t mcu_runmode_normal_obj = {
    { &mcu_runmode_type },
};

const mcu_runmode_obj_t mcu_runmode_safe_mode_obj = {
    { &mcu_runmode_type },
};

const mcu_runmode_obj_t mcu_runmode_bootloader_obj = {
    { &mcu_runmode_type },
};

static const mp_rom_map_elem_t mcu_runmode_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR_UF2),        MP_ROM_PTR(&mcu_runmode_uf2_obj)},
    {MP_ROM_QSTR(MP_QSTR_NORMAL),     MP_ROM_PTR(&mcu_runmode_normal_obj)},
    {MP_ROM_QSTR(MP_QSTR_SAFE_MODE),  MP_ROM_PTR(&mcu_runmode_safe_mode_obj)},
    {MP_ROM_QSTR(MP_QSTR_BOOTLOADER), MP_ROM_PTR(&mcu_runmode_bootloader_obj)},
};
static MP_DEFINE_CONST_DICT(mcu_runmode_locals_dict, mcu_runmode_locals_dict_table);

static void mcu_runmode_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    qstr runmode = MP_QSTR_NORMAL;
    if (self_in == MP_ROM_PTR(&mcu_runmode_uf2_obj)) {
        runmode = MP_QSTR_UF2;
    } else if (self_in == MP_ROM_PTR(&mcu_runmode_safe_mode_obj)) {
        runmode = MP_QSTR_SAFE_MODE;
    } else if (self_in == MP_ROM_PTR(&mcu_runmode_bootloader_obj)) {
        runmode = MP_QSTR_BOOTLOADER;
    }
    mp_printf(print, "%q.%q.%q", MP_QSTR_microcontroller, MP_QSTR_RunMode,
        runmode);
}

MP_DEFINE_CONST_OBJ_TYPE(
    mcu_runmode_type,
    MP_QSTR_RunMode,
    MP_TYPE_FLAG_NONE,
    print, mcu_runmode_print,
    locals_dict, &mcu_runmode_locals_dict
    );
