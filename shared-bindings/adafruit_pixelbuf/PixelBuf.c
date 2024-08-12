// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2018 Rose Hooper
//
// SPDX-License-Identifier: MIT

#include "py/obj.h"
#include "py/objarray.h"
#include "py/objtype.h"
#include "py/mphal.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/objproperty.h"
#include "py/runtime.h"
#include "py/gc.h"

#include <string.h>

#include "shared-bindings/adafruit_pixelbuf/PixelBuf.h"
#include "shared-module/adafruit_pixelbuf/PixelBuf.h"
#include "shared-bindings/digitalio/DigitalInOut.h"

#if CIRCUITPY_ULAB
#include "extmod/ulab/code/ndarray.h"
#endif

static NORETURN void invalid_byteorder(void) {
    mp_arg_error_invalid(MP_QSTR_byteorder);
}

static void parse_byteorder(mp_obj_t byteorder_obj, pixelbuf_byteorder_details_t *parsed);

//| class PixelBuf:
//|     """A fast RGB[W] pixel buffer for LED and similar devices."""
//|
//|     def __init__(
//|         self,
//|         size: int,
//|         *,
//|         byteorder: str = "BGR",
//|         brightness: float = 0,
//|         auto_write: bool = False,
//|         header: ReadableBuffer = b"",
//|         trailer: ReadableBuffer = b""
//|     ) -> None:
//|         """Create a PixelBuf object of the specified size, byteorder, and bits per pixel.
//|
//|         When brightness is less than 1.0, a second buffer will be used to store the color values
//|         before they are adjusted for brightness.
//|
//|         When ``P`` (PWM duration) is present as the 4th character of the byteorder
//|         string, the 4th value in the tuple/list for a pixel is the individual pixel
//|         brightness (0.0-1.0) and will enable a Dotstar compatible 1st byte for each
//|         pixel.
//|
//|         :param int size: Number of pixels
//|         :param str byteorder: Byte order string (such as "RGB", "RGBW" or "PBGR")
//|         :param float brightness: Brightness (0 to 1.0, default 1.0)
//|         :param bool auto_write: Whether to automatically write pixels (Default False)
//|         :param ~circuitpython_typing.ReadableBuffer header: Sequence of bytes to always send before pixel values.
//|         :param ~circuitpython_typing.ReadableBuffer trailer: Sequence of bytes to always send after pixel values.
//|         """
//|         ...
static mp_obj_t pixelbuf_pixelbuf_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_size, ARG_byteorder, ARG_brightness, ARG_auto_write, ARG_header, ARG_trailer };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_size, MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_byteorder, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NEW_QSTR(MP_QSTR_BGR) } },
        { MP_QSTR_brightness, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = mp_const_none } },
        { MP_QSTR_auto_write, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_header, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = mp_const_none } },
        { MP_QSTR_trailer, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = mp_const_none } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    pixelbuf_byteorder_details_t byteorder_details;

    parse_byteorder(args[ARG_byteorder].u_obj, &byteorder_details);

    mp_buffer_info_t header_bufinfo;
    mp_buffer_info_t trailer_bufinfo;

    if (!mp_get_buffer(args[ARG_header].u_obj, &header_bufinfo, MP_BUFFER_READ)) {
        header_bufinfo.buf = NULL;
        header_bufinfo.len = 0;
    }
    if (!mp_get_buffer(args[ARG_trailer].u_obj, &trailer_bufinfo, MP_BUFFER_READ)) {
        trailer_bufinfo.buf = NULL;
        trailer_bufinfo.len = 0;
    }

    float brightness = 1.0;
    if (args[ARG_brightness].u_obj != mp_const_none) {
        brightness = mp_obj_get_float(args[ARG_brightness].u_obj);
        if (brightness < 0) {
            brightness = 0;
        } else if (brightness > 1) {
            brightness = 1;
        }
    }

    // Validation complete, allocate and populate object.
    pixelbuf_pixelbuf_obj_t *self = mp_obj_malloc(pixelbuf_pixelbuf_obj_t, &pixelbuf_pixelbuf_type);
    common_hal_adafruit_pixelbuf_pixelbuf_construct(self, args[ARG_size].u_int,
        &byteorder_details, brightness, args[ARG_auto_write].u_bool, header_bufinfo.buf,
        header_bufinfo.len, trailer_bufinfo.buf, trailer_bufinfo.len);

    return MP_OBJ_FROM_PTR(self);
}

static void parse_byteorder(mp_obj_t byteorder_obj, pixelbuf_byteorder_details_t *parsed) {
    mp_arg_validate_type_string(byteorder_obj, MP_QSTR_byteorder);

    size_t bo_len;
    const char *byteorder = mp_obj_str_get_data(byteorder_obj, &bo_len);
    if (bo_len < 3 || bo_len > 4) {
        invalid_byteorder();
    }
    parsed->order_string = byteorder_obj;

    parsed->bpp = bo_len;
    char *dotstar = strchr(byteorder, 'P');
    char *r = strchr(byteorder, 'R');
    char *g = strchr(byteorder, 'G');
    char *b = strchr(byteorder, 'B');
    char *w = strchr(byteorder, 'W');
    int num_chars = (dotstar ? 1 : 0) + (w ? 1 : 0) + (r ? 1 : 0) + (g ? 1 : 0) + (b ? 1 : 0);
    if ((num_chars < parsed->bpp) || !(r && b && g)) {
        invalid_byteorder();
    }
    parsed->is_dotstar = dotstar ? true : false;
    parsed->has_white = w ? true : false;
    parsed->byteorder.r = r - byteorder;
    parsed->byteorder.g = g - byteorder;
    parsed->byteorder.b = b - byteorder;
    parsed->byteorder.w = w ? w - byteorder : 0;
    // The dotstar brightness byte is always first (as it goes with the pixel start bits)
    if (dotstar && byteorder[0] != 'P') {
        invalid_byteorder();
    }
    if (parsed->has_white && parsed->is_dotstar) {
        invalid_byteorder();
    }
}

//|     bpp: int
//|     """The number of bytes per pixel in the buffer (read-only)"""
static mp_obj_t pixelbuf_pixelbuf_obj_get_bpp(mp_obj_t self_in) {
    return MP_OBJ_NEW_SMALL_INT(common_hal_adafruit_pixelbuf_pixelbuf_get_bpp(self_in));
}
MP_DEFINE_CONST_FUN_OBJ_1(pixelbuf_pixelbuf_get_bpp_obj, pixelbuf_pixelbuf_obj_get_bpp);

MP_PROPERTY_GETTER(pixelbuf_pixelbuf_bpp_obj,
    (mp_obj_t)&pixelbuf_pixelbuf_get_bpp_obj);


//|     brightness: float
//|     """Float value between 0 and 1.  Output brightness.
//|
//|     When brightness is less than 1.0, a second buffer will be used to store the color values
//|     before they are adjusted for brightness."""
static mp_obj_t pixelbuf_pixelbuf_obj_get_brightness(mp_obj_t self_in) {
    return mp_obj_new_float(common_hal_adafruit_pixelbuf_pixelbuf_get_brightness(self_in));
}
MP_DEFINE_CONST_FUN_OBJ_1(pixelbuf_pixelbuf_get_brightness_obj, pixelbuf_pixelbuf_obj_get_brightness);


static mp_obj_t pixelbuf_pixelbuf_obj_set_brightness(mp_obj_t self_in, mp_obj_t value) {
    mp_float_t brightness = mp_obj_get_float(value);
    if (brightness > 1) {
        brightness = 1;
    } else if (brightness < 0) {
        brightness = 0;
    }
    common_hal_adafruit_pixelbuf_pixelbuf_set_brightness(self_in, brightness);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(pixelbuf_pixelbuf_set_brightness_obj, pixelbuf_pixelbuf_obj_set_brightness);

MP_PROPERTY_GETSET(pixelbuf_pixelbuf_brightness_obj,
    (mp_obj_t)&pixelbuf_pixelbuf_get_brightness_obj,
    (mp_obj_t)&pixelbuf_pixelbuf_set_brightness_obj);

//|     auto_write: bool
//|     """Whether to automatically write the pixels after each update."""
static mp_obj_t pixelbuf_pixelbuf_obj_get_auto_write(mp_obj_t self_in) {
    return mp_obj_new_bool(common_hal_adafruit_pixelbuf_pixelbuf_get_auto_write(self_in));
}
MP_DEFINE_CONST_FUN_OBJ_1(pixelbuf_pixelbuf_get_auto_write_obj, pixelbuf_pixelbuf_obj_get_auto_write);


static mp_obj_t pixelbuf_pixelbuf_obj_set_auto_write(mp_obj_t self_in, mp_obj_t value) {
    common_hal_adafruit_pixelbuf_pixelbuf_set_auto_write(self_in, mp_obj_is_true(value));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(pixelbuf_pixelbuf_set_auto_write_obj, pixelbuf_pixelbuf_obj_set_auto_write);

MP_PROPERTY_GETSET(pixelbuf_pixelbuf_auto_write_obj,
    (mp_obj_t)&pixelbuf_pixelbuf_get_auto_write_obj,
    (mp_obj_t)&pixelbuf_pixelbuf_set_auto_write_obj);

//|     byteorder: str
//|     """byteorder string for the buffer (read-only)"""
static mp_obj_t pixelbuf_pixelbuf_obj_get_byteorder(mp_obj_t self_in) {
    return common_hal_adafruit_pixelbuf_pixelbuf_get_byteorder_string(self_in);
}
MP_DEFINE_CONST_FUN_OBJ_1(pixelbuf_pixelbuf_get_byteorder_str, pixelbuf_pixelbuf_obj_get_byteorder);

MP_PROPERTY_GETTER(pixelbuf_pixelbuf_byteorder_str,
    (mp_obj_t)&pixelbuf_pixelbuf_get_byteorder_str);

static mp_obj_t pixelbuf_pixelbuf_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    switch (op) {
        case MP_UNARY_OP_BOOL:
            return mp_const_true;
        case MP_UNARY_OP_LEN:
            return MP_OBJ_NEW_SMALL_INT(common_hal_adafruit_pixelbuf_pixelbuf_get_len(self_in));
        default:
            return MP_OBJ_NULL;      // op not supported
    }
}

//|     def show(self) -> None:
//|         """Transmits the color data to the pixels so that they are shown. This is done automatically
//|         when `auto_write` is True."""
//|         ...

static mp_obj_t pixelbuf_pixelbuf_show(mp_obj_t self_in) {
    common_hal_adafruit_pixelbuf_pixelbuf_show(self_in);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pixelbuf_pixelbuf_show_obj, pixelbuf_pixelbuf_show);

//|     def fill(self, color: PixelType) -> None:
//|         """Fills the given pixelbuf with the given color."""
//|         ...

static mp_obj_t pixelbuf_pixelbuf_fill(mp_obj_t self_in, mp_obj_t value) {
    common_hal_adafruit_pixelbuf_pixelbuf_fill(self_in, value);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(pixelbuf_pixelbuf_fill_obj, pixelbuf_pixelbuf_fill);

//|     @overload
//|     def __getitem__(self, index: slice) -> PixelReturnSequence:
//|         """Returns the pixel value at the given index as a tuple of (Red, Green, Blue[, White]) values
//|         between 0 and 255.  When in PWM (DotStar) mode, the 4th tuple value is a float of the pixel
//|         intensity from 0-1.0."""
//|         ...
//|
//|     @overload
//|     def __getitem__(self, index: int) -> PixelReturnType:
//|         """Returns the pixel value at the given index as a tuple of (Red, Green, Blue[, White]) values
//|         between 0 and 255.  When in PWM (DotStar) mode, the 4th tuple value is a float of the pixel
//|         intensity from 0-1.0."""
//|         ...
//|
//|     @overload
//|     def __setitem__(self, index: slice, value: PixelSequence) -> None: ...
//|     @overload
//|     def __setitem__(self, index: int, value: PixelType) -> None:
//|         """Sets the pixel value at the given index.  Value can either be a tuple or integer.  Tuples are
//|         The individual (Red, Green, Blue[, White]) values between 0 and 255.  If given an integer, the
//|         red, green and blue values are packed into the lower three bytes (0xRRGGBB).
//|         For RGBW byteorders, if given only RGB values either as an int or as a tuple, the white value
//|         is used instead when the red, green, and blue values are the same."""
//|         ...
//|
static mp_obj_t pixelbuf_pixelbuf_subscr(mp_obj_t self_in, mp_obj_t index_in, mp_obj_t value) {
    if (value == MP_OBJ_NULL) {
        // delete item
        // slice deletion
        return MP_OBJ_NULL; // op not supported
    }

    if (0) {
    #if MICROPY_PY_BUILTINS_SLICE
    } else if (mp_obj_is_type(index_in, &mp_type_slice)) {
        mp_bound_slice_t slice;

        size_t length = common_hal_adafruit_pixelbuf_pixelbuf_get_len(self_in);
        mp_seq_get_fast_slice_indexes(length, index_in, &slice);
        static mp_obj_tuple_t flat_item_tuple = {
            .base = {&mp_type_tuple},
            .len = 0,
            .items = {
                mp_const_none,
                mp_const_none,
                mp_const_none,
                mp_const_none,
            }
        };

        size_t slice_len;
        if (slice.step > 0) {
            slice_len = slice.stop - slice.start;
        } else {
            slice_len = 1 + slice.start - slice.stop;
        }
        if (slice.step > 1 || slice.step < -1) {
            size_t step = slice.step > 0 ? slice.step : slice.step * -1;
            slice_len = (slice_len / step) + (slice_len % step ? 1 : 0);
        }

        if (value == MP_OBJ_SENTINEL) { // Get
            mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(slice_len, NULL));
            for (uint i = 0; i < slice_len; i++) {
                t->items[i] = common_hal_adafruit_pixelbuf_pixelbuf_get_pixel(self_in, i * slice.step + slice.start);
            }
            return MP_OBJ_FROM_PTR(t);
        } else { // Set
            #if MICROPY_PY_ARRAY_SLICE_ASSIGN

            size_t num_items = mp_obj_get_int(mp_obj_len(value));

            if (num_items != slice_len && num_items != (slice_len * common_hal_adafruit_pixelbuf_pixelbuf_get_bpp(self_in))) {
                mp_raise_ValueError_varg(MP_ERROR_TEXT("Unmatched number of items on RHS (expected %d, got %d)."), slice_len, num_items);
            }
            common_hal_adafruit_pixelbuf_pixelbuf_set_pixels(self_in, slice.start, slice.step, slice_len, value,
                num_items != slice_len ? &flat_item_tuple : mp_const_none);
            return mp_const_none;
            #else
            return MP_OBJ_NULL; // op not supported
            #endif
        }
    #endif
    } else { // Single index rather than slice.
        size_t length = common_hal_adafruit_pixelbuf_pixelbuf_get_len(self_in);
        size_t index = mp_get_index(mp_obj_get_type(self_in), length, index_in, false);

        if (value == MP_OBJ_SENTINEL) { // Get
            return common_hal_adafruit_pixelbuf_pixelbuf_get_pixel(self_in, index);
        } else { // Store
            common_hal_adafruit_pixelbuf_pixelbuf_set_pixel(self_in, index, value);
            return mp_const_none;
        }
    }
}

static const mp_rom_map_elem_t pixelbuf_pixelbuf_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_auto_write), MP_ROM_PTR(&pixelbuf_pixelbuf_auto_write_obj)},
    { MP_ROM_QSTR(MP_QSTR_bpp), MP_ROM_PTR(&pixelbuf_pixelbuf_bpp_obj)},
    { MP_ROM_QSTR(MP_QSTR_brightness), MP_ROM_PTR(&pixelbuf_pixelbuf_brightness_obj)},
    { MP_ROM_QSTR(MP_QSTR_byteorder), MP_ROM_PTR(&pixelbuf_pixelbuf_byteorder_str)},
    { MP_ROM_QSTR(MP_QSTR_show), MP_ROM_PTR(&pixelbuf_pixelbuf_show_obj)},
    { MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&pixelbuf_pixelbuf_fill_obj)},
};

static MP_DEFINE_CONST_DICT(pixelbuf_pixelbuf_locals_dict, pixelbuf_pixelbuf_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    pixelbuf_pixelbuf_type,
    MP_QSTR_PixelBuf,
    MP_TYPE_FLAG_ITER_IS_GETITER | MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    locals_dict, &pixelbuf_pixelbuf_locals_dict,
    make_new, pixelbuf_pixelbuf_make_new,
    subscr, pixelbuf_pixelbuf_subscr,
    unary_op, pixelbuf_pixelbuf_unary_op,
    iter, mp_obj_generic_subscript_getiter
    );
