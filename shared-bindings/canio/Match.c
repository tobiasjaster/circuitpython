// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Jeff Epler for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/canio/Match.h"

#include "py/objproperty.h"
#include "py/runtime.h"

//| class Match:
//|     """Describe CAN bus messages to match"""
//|
//|     def __init__(self, id: int, *, mask: Optional[int] = None, extended: bool = False) -> None:
//|         """Construct a Match with the given properties.
//|
//|         If mask is not None, then the filter is for any id which matches all
//|         the nonzero bits in mask. Otherwise, it matches exactly the given id.
//|         If extended is true then only extended ids are matched, otherwise
//|         only standard ids are matched."""

static mp_obj_t canio_match_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_id, ARG_mask, ARG_extended, NUM_ARGS };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_mask, MP_ARG_OBJ, {.u_obj = mp_const_none } },
        { MP_QSTR_extended, MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    MP_STATIC_ASSERT(MP_ARRAY_SIZE(allowed_args) == NUM_ARGS);

    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    int id_bits = args[ARG_extended].u_bool ? 0x1fffffff : 0x7ff;
    int id = args[ARG_id].u_int;
    int mask = args[ARG_mask].u_obj == mp_const_none ?  id_bits : mp_obj_get_int(args[ARG_mask].u_obj);

    if (id & ~id_bits) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("%q out of range"), MP_QSTR_id);
    }

    if (mask & ~id_bits) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("%q out of range"), MP_QSTR_mask);
    }

    canio_match_obj_t *self = mp_obj_malloc(canio_match_obj_t, &canio_match_type);
    common_hal_canio_match_construct(self, id, mask, args[ARG_extended].u_bool);
    return self;
}

//|     id: int
//|     """The id to match"""

static mp_obj_t canio_match_id_get(mp_obj_t self_in) {
    canio_match_obj_t *self = self_in;
    return MP_OBJ_NEW_SMALL_INT(common_hal_canio_match_get_id(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(canio_match_id_get_obj, canio_match_id_get);

MP_PROPERTY_GETTER(canio_match_id_obj,
    (mp_obj_t)&canio_match_id_get_obj);

//|     mask: int
//|     """The optional mask of ids to match"""

static mp_obj_t canio_match_mask_get(mp_obj_t self_in) {
    canio_match_obj_t *self = self_in;
    return MP_OBJ_NEW_SMALL_INT(common_hal_canio_match_get_mask(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(canio_match_mask_get_obj, canio_match_mask_get);

MP_PROPERTY_GETTER(canio_match_mask_obj,
    (mp_obj_t)&canio_match_mask_get_obj);

//|     extended: bool
//|     """True to match extended ids, False to match standard ides"""
//|

static mp_obj_t canio_match_extended_get(mp_obj_t self_in) {
    canio_match_obj_t *self = self_in;
    return mp_obj_new_bool(common_hal_canio_match_get_extended(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(canio_match_extended_get_obj, canio_match_extended_get);

MP_PROPERTY_GETTER(canio_match_extended_obj,
    (mp_obj_t)&canio_match_extended_get_obj);

static const mp_rom_map_elem_t canio_match_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_id), MP_ROM_PTR(&canio_match_id_obj) },
    { MP_ROM_QSTR(MP_QSTR_mask), MP_ROM_PTR(&canio_match_mask_obj) },
    { MP_ROM_QSTR(MP_QSTR_extended), MP_ROM_PTR(&canio_match_extended_obj) },
};
static MP_DEFINE_CONST_DICT(canio_match_locals_dict, canio_match_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    canio_match_type,
    MP_QSTR_Match,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    make_new, canio_match_make_new,
    locals_dict, &canio_match_locals_dict
    );
