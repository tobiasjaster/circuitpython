// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2018 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/displayio/Group.h"

#include <stdint.h>

#include "shared/runtime/context_manager_helpers.h"
#include "py/binary.h"
#include "py/objproperty.h"
#include "py/objtype.h"
#include "py/runtime.h"

//| class Group:
//|     """Manage a group of sprites and groups and how they are inter-related."""
//|
//|     def __init__(self, *, scale: int = 1, x: int = 0, y: int = 0) -> None:
//|         """Create a Group of a given size and scale. Scale is in one dimension. For example, scale=2
//|         leads to a layer's pixel being 2x2 pixels when in the group.
//|
//|         :param int scale: Scale of layer pixels in one dimension.
//|         :param int x: Initial x position within the parent.
//|         :param int y: Initial y position within the parent."""
//|         ...
static mp_obj_t displayio_group_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_scale, ARG_x, ARG_y };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_scale, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = 1} },
        { MP_QSTR_x, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = 0} },
        { MP_QSTR_y, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = 0} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_int_t scale = mp_arg_validate_int_range(args[ARG_scale].u_int, 1, 32767, MP_QSTR_scale);

    displayio_group_t *self = mp_obj_malloc(displayio_group_t, &displayio_group_type);
    common_hal_displayio_group_construct(self, scale, args[ARG_x].u_int, args[ARG_y].u_int);

    return MP_OBJ_FROM_PTR(self);
}

// Helper to ensure we have the native super class instead of a subclass.
displayio_group_t *native_group(mp_obj_t group_obj) {
    mp_obj_t native_group = mp_obj_cast_to_native_base(group_obj, &displayio_group_type);
    if (native_group == MP_OBJ_NULL) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("Must be a %q subclass."), MP_QSTR_Group);
    }
    mp_obj_assert_native_inited(native_group);
    return MP_OBJ_TO_PTR(native_group);
}

//|     hidden: bool
//|     """True when the Group and all of its layers are not visible. When False, the Group's layers
//|     are visible if they haven't been hidden."""
static mp_obj_t displayio_group_obj_get_hidden(mp_obj_t self_in) {
    displayio_group_t *self = native_group(self_in);
    return mp_obj_new_bool(common_hal_displayio_group_get_hidden(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(displayio_group_get_hidden_obj, displayio_group_obj_get_hidden);

static mp_obj_t displayio_group_obj_set_hidden(mp_obj_t self_in, mp_obj_t hidden_obj) {
    displayio_group_t *self = native_group(self_in);

    common_hal_displayio_group_set_hidden(self, mp_obj_is_true(hidden_obj));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(displayio_group_set_hidden_obj, displayio_group_obj_set_hidden);

MP_PROPERTY_GETSET(displayio_group_hidden_obj,
    (mp_obj_t)&displayio_group_get_hidden_obj,
    (mp_obj_t)&displayio_group_set_hidden_obj);

//|     scale: int
//|     """Scales each pixel within the Group in both directions. For example, when scale=2 each pixel
//|     will be represented by 2x2 pixels."""
static mp_obj_t displayio_group_obj_get_scale(mp_obj_t self_in) {
    displayio_group_t *self = native_group(self_in);
    return MP_OBJ_NEW_SMALL_INT(common_hal_displayio_group_get_scale(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(displayio_group_get_scale_obj, displayio_group_obj_get_scale);

static mp_obj_t displayio_group_obj_set_scale(mp_obj_t self_in, mp_obj_t scale_obj) {
    displayio_group_t *self = native_group(self_in);

    mp_int_t scale = mp_arg_validate_int_min(mp_obj_get_int(scale_obj), 1, MP_QSTR_scale);

    common_hal_displayio_group_set_scale(self, scale);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(displayio_group_set_scale_obj, displayio_group_obj_set_scale);

MP_PROPERTY_GETSET(displayio_group_scale_obj,
    (mp_obj_t)&displayio_group_get_scale_obj,
    (mp_obj_t)&displayio_group_set_scale_obj);

//|     x: int
//|     """X position of the Group in the parent."""
static mp_obj_t displayio_group_obj_get_x(mp_obj_t self_in) {
    displayio_group_t *self = native_group(self_in);
    return MP_OBJ_NEW_SMALL_INT(common_hal_displayio_group_get_x(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(displayio_group_get_x_obj, displayio_group_obj_get_x);

static mp_obj_t displayio_group_obj_set_x(mp_obj_t self_in, mp_obj_t x_obj) {
    displayio_group_t *self = native_group(self_in);

    mp_int_t x = mp_obj_get_int(x_obj);
    common_hal_displayio_group_set_x(self, x);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(displayio_group_set_x_obj, displayio_group_obj_set_x);

MP_PROPERTY_GETSET(displayio_group_x_obj,
    (mp_obj_t)&displayio_group_get_x_obj,
    (mp_obj_t)&displayio_group_set_x_obj);

//|     y: int
//|     """Y position of the Group in the parent."""
static mp_obj_t displayio_group_obj_get_y(mp_obj_t self_in) {
    displayio_group_t *self = native_group(self_in);
    return MP_OBJ_NEW_SMALL_INT(common_hal_displayio_group_get_y(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(displayio_group_get_y_obj, displayio_group_obj_get_y);

static mp_obj_t displayio_group_obj_set_y(mp_obj_t self_in, mp_obj_t y_obj) {
    displayio_group_t *self = native_group(self_in);

    mp_int_t y = mp_obj_get_int(y_obj);
    common_hal_displayio_group_set_y(self, y);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(displayio_group_set_y_obj, displayio_group_obj_set_y);

MP_PROPERTY_GETSET(displayio_group_y_obj,
    (mp_obj_t)&displayio_group_get_y_obj,
    (mp_obj_t)&displayio_group_set_y_obj);

//|     def append(
//|         self,
//|         layer: Union[vectorio.Circle, vectorio.Rectangle, vectorio.Polygon, Group, TileGrid],
//|     ) -> None:
//|         """Append a layer to the group. It will be drawn above other layers."""
//|         ...
static mp_obj_t displayio_group_obj_append(mp_obj_t self_in, mp_obj_t layer) {
    displayio_group_t *self = native_group(self_in);
    common_hal_displayio_group_insert(self, common_hal_displayio_group_get_len(self), layer);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(displayio_group_append_obj, displayio_group_obj_append);

//|     def insert(
//|         self,
//|         index: int,
//|         layer: Union[vectorio.Circle, vectorio.Rectangle, vectorio.Polygon, Group, TileGrid],
//|     ) -> None:
//|         """Insert a layer into the group."""
//|         ...
static mp_obj_t displayio_group_obj_insert(mp_obj_t self_in, mp_obj_t index_obj, mp_obj_t layer) {
    displayio_group_t *self = native_group(self_in);
    if ((size_t)MP_OBJ_SMALL_INT_VALUE(index_obj) == common_hal_displayio_group_get_len(self)) {
        return displayio_group_obj_append(self_in, layer);
    }
    size_t index = mp_get_index(&displayio_group_type, common_hal_displayio_group_get_len(self), index_obj, false);
    common_hal_displayio_group_insert(self, index, layer);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(displayio_group_insert_obj, displayio_group_obj_insert);


//|     def index(
//|         self,
//|         layer: Union[vectorio.Circle, vectorio.Rectangle, vectorio.Polygon, Group, TileGrid],
//|     ) -> int:
//|         """Returns the index of the first copy of layer. Raises ValueError if not found."""
//|         ...
static mp_obj_t displayio_group_obj_index(mp_obj_t self_in, mp_obj_t layer) {
    displayio_group_t *self = native_group(self_in);
    mp_int_t index = common_hal_displayio_group_index(self, layer);
    if (index < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("object not in sequence"));
    }
    return MP_OBJ_NEW_SMALL_INT(index);
}
MP_DEFINE_CONST_FUN_OBJ_2(displayio_group_index_obj, displayio_group_obj_index);

//|     def pop(
//|         self, i: int = -1
//|     ) -> Union[vectorio.Circle, vectorio.Rectangle, vectorio.Polygon, Group, TileGrid]:
//|         """Remove the ith item and return it."""
//|         ...
static mp_obj_t displayio_group_obj_pop(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_i };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_i, MP_ARG_INT, {.u_int = -1} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    displayio_group_t *self = native_group(pos_args[0]);

    size_t index = mp_get_index(&displayio_group_type,
        common_hal_displayio_group_get_len(self),
        MP_OBJ_NEW_SMALL_INT(args[ARG_i].u_int),
        false);
    return common_hal_displayio_group_pop(self, index);
}
MP_DEFINE_CONST_FUN_OBJ_KW(displayio_group_pop_obj, 1, displayio_group_obj_pop);


//|     def remove(
//|         self,
//|         layer: Union[vectorio.Circle, vectorio.Rectangle, vectorio.Polygon, Group, TileGrid],
//|     ) -> None:
//|         """Remove the first copy of layer. Raises ValueError if it is not present."""
//|         ...
static mp_obj_t displayio_group_obj_remove(mp_obj_t self_in, mp_obj_t layer) {
    mp_obj_t index = displayio_group_obj_index(self_in, layer);
    displayio_group_t *self = native_group(self_in);

    common_hal_displayio_group_pop(self, MP_OBJ_SMALL_INT_VALUE(index));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(displayio_group_remove_obj, displayio_group_obj_remove);

//|     def __bool__(self) -> bool: ...
//|     def __contains__(
//|         self,
//|         item: Union[vectorio.Circle, vectorio.Rectangle, vectorio.Polygon, Group, TileGrid],
//|     ) -> bool: ...
//|     def __iter__(
//|         self,
//|     ) -> Iterator[
//|         Union[vectorio.Circle, vectorio.Rectangle, vectorio.Polygon, Group, TileGrid]
//|     ]: ...
//|     def __len__(self) -> int:
//|         """Returns the number of layers in a Group"""
//|         ...
static mp_obj_t group_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    displayio_group_t *self = native_group(self_in);
    uint16_t len = common_hal_displayio_group_get_len(self);
    switch (op) {
        case MP_UNARY_OP_BOOL:
            return mp_obj_new_bool(len != 0);
        case MP_UNARY_OP_LEN:
            return MP_OBJ_NEW_SMALL_INT(len);
        default:
            return MP_OBJ_NULL;      // op not supported
    }
}

//|     def __getitem__(
//|         self, index: int
//|     ) -> Union[vectorio.Circle, vectorio.Rectangle, vectorio.Polygon, Group, TileGrid]:
//|         """Returns the value at the given index.
//|
//|         This allows you to::
//|
//|           print(group[0])"""
//|         ...
//|
//|     def __setitem__(
//|         self,
//|         index: int,
//|         value: Union[vectorio.Circle, vectorio.Rectangle, vectorio.Polygon, Group, TileGrid],
//|     ) -> None:
//|         """Sets the value at the given index.
//|
//|         This allows you to::
//|
//|           group[0] = sprite"""
//|         ...
//|
//|     def __delitem__(self, index: int) -> None:
//|         """Deletes the value at the given index.
//|
//|         This allows you to::
//|
//|           del group[0]"""
//|         ...
static mp_obj_t group_subscr(mp_obj_t self_in, mp_obj_t index_obj, mp_obj_t value) {
    displayio_group_t *self = native_group(self_in);

    if (mp_obj_is_type(index_obj, &mp_type_slice)) {
        mp_raise_NotImplementedError(MP_ERROR_TEXT("Slices not supported"));
    } else {
        size_t index = mp_get_index(&displayio_group_type, common_hal_displayio_group_get_len(self), index_obj, false);

        if (value == MP_OBJ_SENTINEL) {
            // load
            return common_hal_displayio_group_get(self, index);
        } else if (value == MP_OBJ_NULL) {
            common_hal_displayio_group_pop(self, index);
        } else {
            common_hal_displayio_group_set(self, index, value);
        }
    }
    return mp_const_none;
}

//|     def sort(self, key: function, reverse: bool) -> None:
//|         """Sort the members of the group."""
//|         ...
//|
static mp_obj_t displayio_group_obj_sort(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    displayio_group_t *self = native_group(pos_args[0]);
    mp_obj_t *args = m_new(mp_obj_t, n_args);
    for (size_t i = 1; i < n_args; ++i) {
        args[i] = pos_args[i];
    }
    args[0] = MP_OBJ_FROM_PTR(self->members);
    return mp_obj_list_sort(n_args, args, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(displayio_group_sort_obj, 1, displayio_group_obj_sort);

static const mp_rom_map_elem_t displayio_group_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_hidden), MP_ROM_PTR(&displayio_group_hidden_obj) },
    { MP_ROM_QSTR(MP_QSTR_scale), MP_ROM_PTR(&displayio_group_scale_obj) },
    { MP_ROM_QSTR(MP_QSTR_x), MP_ROM_PTR(&displayio_group_x_obj) },
    { MP_ROM_QSTR(MP_QSTR_y), MP_ROM_PTR(&displayio_group_y_obj) },
    { MP_ROM_QSTR(MP_QSTR_append), MP_ROM_PTR(&displayio_group_append_obj) },
    { MP_ROM_QSTR(MP_QSTR_insert), MP_ROM_PTR(&displayio_group_insert_obj) },
    { MP_ROM_QSTR(MP_QSTR_index), MP_ROM_PTR(&displayio_group_index_obj) },
    { MP_ROM_QSTR(MP_QSTR_pop), MP_ROM_PTR(&displayio_group_pop_obj) },
    { MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&displayio_group_remove_obj) },
    { MP_ROM_QSTR(MP_QSTR_sort), MP_ROM_PTR(&displayio_group_sort_obj) },
};
static MP_DEFINE_CONST_DICT(displayio_group_locals_dict, displayio_group_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    displayio_group_type,
    MP_QSTR_Group,
    MP_TYPE_FLAG_ITER_IS_GETITER | MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    make_new, displayio_group_make_new,
    locals_dict, &displayio_group_locals_dict,
    subscr, group_subscr,
    unary_op, group_unary_op,
    iter, mp_obj_generic_subscript_getiter
    );
