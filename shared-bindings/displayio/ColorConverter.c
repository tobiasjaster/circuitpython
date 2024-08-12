// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2018 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/displayio/ColorConverter.h"

#include <stdint.h>

#include "shared/runtime/context_manager_helpers.h"
#include "py/binary.h"
#include "py/enum.h"
#include "py/objproperty.h"
#include "py/runtime.h"
#include "shared-bindings/util.h"

//| class ColorConverter:
//|     """Converts one color format to another."""
//|
//|     def __init__(
//|         self, *, input_colorspace: Colorspace = Colorspace.RGB888, dither: bool = False
//|     ) -> None:
//|         """Create a ColorConverter object to convert color formats.
//|
//|         :param Colorspace colorspace: The source colorspace, one of the Colorspace constants
//|         :param bool dither: Adds random noise to dither the output image"""
//|         ...

static mp_obj_t displayio_colorconverter_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_dither, ARG_input_colorspace };

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_dither, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_input_colorspace, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = (void *)&displayio_colorspace_RGB888_obj} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    displayio_colorconverter_t *self = mp_obj_malloc(displayio_colorconverter_t, &displayio_colorconverter_type);

    common_hal_displayio_colorconverter_construct(self, args[ARG_dither].u_bool, (displayio_colorspace_t)cp_enum_value(&displayio_colorspace_type, args[ARG_input_colorspace].u_obj, MP_QSTR_input_colorspace));

    return MP_OBJ_FROM_PTR(self);
}

//|     def convert(self, color: int) -> int:
//|         """Converts the given color to RGB565 according to the Colorspace"""
//|         ...
static mp_obj_t displayio_colorconverter_obj_convert(mp_obj_t self_in, mp_obj_t color_obj) {
    displayio_colorconverter_t *self = MP_OBJ_TO_PTR(self_in);

    mp_int_t color = mp_arg_validate_type_int(color_obj, MP_QSTR_color);
    uint32_t output_color;
    common_hal_displayio_colorconverter_convert(self, &self->output_colorspace, color, &output_color);
    return MP_OBJ_NEW_SMALL_INT(output_color);
}
MP_DEFINE_CONST_FUN_OBJ_2(displayio_colorconverter_convert_obj, displayio_colorconverter_obj_convert);

//|     dither: bool
//|     """When `True` the ColorConverter dithers the output by adding random noise when
//|     truncating to display bitdepth"""
static mp_obj_t displayio_colorconverter_obj_get_dither(mp_obj_t self_in) {
    displayio_colorconverter_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(common_hal_displayio_colorconverter_get_dither(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(displayio_colorconverter_get_dither_obj, displayio_colorconverter_obj_get_dither);

static mp_obj_t displayio_colorconverter_obj_set_dither(mp_obj_t self_in, mp_obj_t dither) {
    displayio_colorconverter_t *self = MP_OBJ_TO_PTR(self_in);

    common_hal_displayio_colorconverter_set_dither(self, mp_obj_is_true(dither));

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(displayio_colorconverter_set_dither_obj, displayio_colorconverter_obj_set_dither);

MP_PROPERTY_GETSET(displayio_colorconverter_dither_obj,
    (mp_obj_t)&displayio_colorconverter_get_dither_obj,
    (mp_obj_t)&displayio_colorconverter_set_dither_obj);

//|     def make_transparent(self, color: int) -> None:
//|         """Set the transparent color or index for the ColorConverter. This will
//|         raise an Exception if there is already a selected transparent index.
//|
//|         :param int color: The color to be transparent"""
static mp_obj_t displayio_colorconverter_make_transparent(mp_obj_t self_in, mp_obj_t transparent_color_obj) {
    displayio_colorconverter_t *self = MP_OBJ_TO_PTR(self_in);

    mp_int_t transparent_color = mp_obj_get_int(transparent_color_obj);
    common_hal_displayio_colorconverter_make_transparent(self, transparent_color);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(displayio_colorconverter_make_transparent_obj, displayio_colorconverter_make_transparent);

//|     def make_opaque(self, color: int) -> None:
//|         """Make the ColorConverter be opaque and have no transparent pixels.
//|
//|         :param int color: [IGNORED] Use any value"""
//|
static mp_obj_t displayio_colorconverter_make_opaque(mp_obj_t self_in, mp_obj_t transparent_color_obj) {
    displayio_colorconverter_t *self = MP_OBJ_TO_PTR(self_in);

    mp_int_t transparent_color = mp_obj_get_int(transparent_color_obj);
    common_hal_displayio_colorconverter_make_opaque(self, transparent_color);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(displayio_colorconverter_make_opaque_obj, displayio_colorconverter_make_opaque);

static const mp_rom_map_elem_t displayio_colorconverter_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_convert), MP_ROM_PTR(&displayio_colorconverter_convert_obj) },
    { MP_ROM_QSTR(MP_QSTR_dither), MP_ROM_PTR(&displayio_colorconverter_dither_obj) },
    { MP_ROM_QSTR(MP_QSTR_make_transparent), MP_ROM_PTR(&displayio_colorconverter_make_transparent_obj) },
    { MP_ROM_QSTR(MP_QSTR_make_opaque), MP_ROM_PTR(&displayio_colorconverter_make_opaque_obj) },
};
static MP_DEFINE_CONST_DICT(displayio_colorconverter_locals_dict, displayio_colorconverter_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    displayio_colorconverter_type,
    MP_QSTR_ColorConverter,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    make_new, displayio_colorconverter_make_new,
    locals_dict, &displayio_colorconverter_locals_dict
    );
