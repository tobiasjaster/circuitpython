// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Radomir Dopieralski
//
// SPDX-License-Identifier: MIT

#include "py/runtime.h"

#include "__init__.h"
#include "Layer.h"

//| class Layer:
//|     """Keep information about a single layer of graphics"""
//|
//|     def __init__(
//|         self,
//|         width: int,
//|         height: int,
//|         graphic: ReadableBuffer,
//|         palette: ReadableBuffer,
//|         grid: ReadableBuffer,
//|     ) -> None:
//|         """Keep internal information about a layer of graphics (either a
//|         ``Grid`` or a ``Sprite``) in a format suitable for fast rendering
//|         with the ``render()`` function.
//|
//|         :param int width: The width of the grid in tiles, or 1 for sprites.
//|         :param int height: The height of the grid in tiles, or 1 for sprites.
//|         :param ~circuitpython_typing.ReadableBuffer graphic: The graphic data of the tiles.
//|         :param ~circuitpython_typing.ReadableBuffer palette: The color palette to be used.
//|         :param ~circuitpython_typing.ReadableBuffer grid: The contents of the grid map.
//|
//|         This class is intended for internal use in the ``stage`` library and
//|         it shouldn't be used on its own."""
//|         ...
static mp_obj_t layer_make_new(const mp_obj_type_t *type, size_t n_args,
    size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 4, 5, false);

    layer_obj_t *self = mp_obj_malloc(layer_obj_t, type);

    self->width = mp_obj_get_int(args[0]);
    self->height = mp_obj_get_int(args[1]);
    self->x = 0;
    self->y = 0;
    self->frame = 0;
    self->rotation = false;

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_READ);
    self->graphic = bufinfo.buf;
    if (bufinfo.len != 2048) {
        mp_raise_ValueError(MP_ERROR_TEXT("graphic must be 2048 bytes long"));
    }

    mp_get_buffer_raise(args[3], &bufinfo, MP_BUFFER_READ);
    self->palette = bufinfo.buf;
    if (bufinfo.len != 32) {
        mp_raise_ValueError(MP_ERROR_TEXT("palette must be 32 bytes long"));
    }

    if (n_args > 4) {
        mp_get_buffer_raise(args[4], &bufinfo, MP_BUFFER_READ);
        self->map = bufinfo.buf;
        if (bufinfo.len < (self->width * self->height) / 2) {
            mp_raise_ValueError(MP_ERROR_TEXT("map buffer too small"));
        }
    } else {
        self->map = NULL;
    }

    return MP_OBJ_FROM_PTR(self);
}

//|     def move(self, x: int, y: int) -> None:
//|         """Set the offset of the layer to the specified values."""
//|         ...
static mp_obj_t layer_move(mp_obj_t self_in, mp_obj_t x_in, mp_obj_t y_in) {
    layer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->x = mp_obj_get_int(x_in);
    self->y = mp_obj_get_int(y_in);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(layer_move_obj, layer_move);

//|     def frame(self, frame: int, rotation: int) -> None:
//|         """Set the animation frame of the sprite, and optionally rotation its
//|         graphic."""
//|         ...
//|
static mp_obj_t layer_frame(mp_obj_t self_in, mp_obj_t frame_in,
    mp_obj_t rotation_in) {
    layer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->frame = mp_obj_get_int(frame_in);
    self->rotation = mp_obj_get_int(rotation_in);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(layer_frame_obj, layer_frame);


static const mp_rom_map_elem_t layer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_move), MP_ROM_PTR(&layer_move_obj) },
    { MP_ROM_QSTR(MP_QSTR_frame), MP_ROM_PTR(&layer_frame_obj) },
};
static MP_DEFINE_CONST_DICT(layer_locals_dict, layer_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_layer,
    MP_QSTR_Layer,
    MP_TYPE_FLAG_NONE,
    make_new, layer_make_new,
    locals_dict, &layer_locals_dict
    );
