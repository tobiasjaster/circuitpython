// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "py/objproperty.h"
#include "py/runtime.h"
#include "py/runtime0.h"
#include "shared-bindings/memorymonitor/AllocationAlarm.h"
#include "shared-bindings/util.h"

//| class AllocationAlarm:
//|     def __init__(self, *, minimum_block_count: int = 1) -> None:
//|         """Throw an exception when an allocation of ``minimum_block_count`` or more blocks
//|            occurs while active.
//|
//|         Track allocations::
//|
//|           import memorymonitor
//|
//|           aa = memorymonitor.AllocationAlarm(minimum_block_count=2)
//|           x = 2
//|           # Should not allocate any blocks.
//|           with aa:
//|               x = 5
//|
//|           # Should throw an exception when allocating storage for the 20 bytes.
//|           with aa:
//|               x = bytearray(20)
//|
//|         """
//|         ...
static mp_obj_t memorymonitor_allocationalarm_make_new(const mp_obj_type_t *type, size_t n_args, const mp_obj_t *all_args, mp_map_t *kw_args) {
    enum { ARG_minimum_block_count };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_minimum_block_count, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_int_t minimum_block_count =
        mp_arg_validate_int_min(args[ARG_minimum_block_count].u_int, 1, MP_QSTR_minimum_block_count);


    memorymonitor_allocationalarm_obj_t *self =
        mp_obj_malloc(memorymonitor_allocationalarm_obj_t, &memorymonitor_allocationalarm_type);

    common_hal_memorymonitor_allocationalarm_construct(self, minimum_block_count);

    return MP_OBJ_FROM_PTR(self);
}

//|     def ignore(self, count: int) -> AllocationAlarm:
//|         """Sets the number of applicable allocations to ignore before raising the exception.
//|         Automatically set back to zero at context exit.
//|
//|         Use it within a ``with`` block::
//|
//|           # Will not alarm because the bytearray allocation will be ignored.
//|           with aa.ignore(2):
//|               x = bytearray(20)
//|         """
//|         ...
static mp_obj_t memorymonitor_allocationalarm_obj_ignore(mp_obj_t self_in, mp_obj_t count_obj) {
    mp_int_t count = mp_obj_get_int(count_obj);
    mp_arg_validate_int_min(count, 0, MP_QSTR_count);

    common_hal_memorymonitor_allocationalarm_set_ignore(self_in, count);
    return self_in;
}
MP_DEFINE_CONST_FUN_OBJ_2(memorymonitor_allocationalarm_ignore_obj, memorymonitor_allocationalarm_obj_ignore);

//|     def __enter__(self) -> AllocationAlarm:
//|         """Enables the alarm."""
//|         ...
static mp_obj_t memorymonitor_allocationalarm_obj___enter__(mp_obj_t self_in) {
    common_hal_memorymonitor_allocationalarm_resume(self_in);
    return self_in;
}
MP_DEFINE_CONST_FUN_OBJ_1(memorymonitor_allocationalarm___enter___obj, memorymonitor_allocationalarm_obj___enter__);

//|     def __exit__(self) -> None:
//|         """Automatically disables the allocation alarm when exiting a context. See
//|         :ref:`lifetime-and-contextmanagers` for more info."""
//|         ...
//|
static mp_obj_t memorymonitor_allocationalarm_obj___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    common_hal_memorymonitor_allocationalarm_set_ignore(args[0], 0);
    common_hal_memorymonitor_allocationalarm_pause(args[0]);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(memorymonitor_allocationalarm___exit___obj, 4, 4, memorymonitor_allocationalarm_obj___exit__);

static const mp_rom_map_elem_t memorymonitor_allocationalarm_locals_dict_table[] = {
    // Methods
    { MP_ROM_QSTR(MP_QSTR_ignore), MP_ROM_PTR(&memorymonitor_allocationalarm_ignore_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&memorymonitor_allocationalarm___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&memorymonitor_allocationalarm___exit___obj) },
};
static MP_DEFINE_CONST_DICT(memorymonitor_allocationalarm_locals_dict, memorymonitor_allocationalarm_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    memorymonitor_allocationalarm_type,
    MP_QSTR_AllocationAlarm,
    MP_TYPE_FLAG_NONE,
    make_new, memorymonitor_allocationalarm_make_new,
    locals_dict, &memorymonitor_allocationalarm_locals_dict
    );
