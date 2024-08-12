// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 Mark Komus
//
// SPDX-License-Identifier: MIT

#include "py/obj.h"
#include "py/objproperty.h"
#include "py/runtime.h"
#include "py/objarray.h"

#include "shared-bindings/is31fl3741/IS31FL3741.h"
#include "shared-bindings/is31fl3741/FrameBuffer.h"
#include "shared-bindings/util.h"
#include "shared-module/displayio/__init__.h"
#include "shared-module/framebufferio/__init__.h"
#include "shared-module/framebufferio/FramebufferDisplay.h"
#include "shared-bindings/busio/I2C.h"

//| class IS31FL3741_FrameBuffer:
//|     """Creates an in-memory framebuffer for a IS31FL3741 device."""
//|
//|     def __init__(
//|         self,
//|         is31: is31fl3741.IS31FL3741,
//|         width: int,
//|         height: int,
//|         mapping: Tuple[int, ...],
//|         *,
//|         framebuffer: Optional[WriteableBuffer] = None,
//|         scale: bool = False,
//|         gamma: bool = False
//|     ) -> None:
//|         """Create a IS31FL3741_FrameBuffer object with the given attributes.
//|
//|         The framebuffer is in "RGB888" format using 4 bytes per pixel.
//|         Bits 24-31 are ignored. The format is in RGB order.
//|
//|         If a framebuffer is not passed in, one is allocated and initialized
//|         to all black.  In any case, the framebuffer can be retrieved
//|         by passing the Is31fl3741 object to memoryview().
//|
//|         A Is31fl3741 is often used in conjunction with a
//|         `framebufferio.FramebufferDisplay`.
//|
//|         :param is31fl3741.IS31FL3741 is31: base IS31FL3741 instance to drive the framebuffer
//|         :param int width: width of the display
//|         :param int height: height of the display
//|         :param Tuple[int, ...] mapping: mapping of matrix locations to LEDs
//|         :param Optional[WriteableBuffer] framebuffer: Optional buffer to hold the display
//|         :param bool scale: if True display is scaled down by 3 when displayed
//|         :param bool gamma: if True apply gamma correction to all LEDs"""
//|         ...
static mp_obj_t is31fl3741_framebuffer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_is31, ARG_width, ARG_height, ARG_mapping, ARG_framebuffer, ARG_scale, ARG_gamma };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_is31, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_width, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_height, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_mapping, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_framebuffer, MP_ARG_OBJ | MP_ARG_KW_ONLY, { .u_obj = mp_const_none } },
        { MP_QSTR_scale, MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false } },
        { MP_QSTR_gamma, MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    is31fl3741_framebuffer_obj_t *self = &allocate_display_bus_or_raise()->is31fl3741;
    self->base.type = &is31fl3741_framebuffer_type;

    if (args[ARG_width].u_int <= 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("width must be greater than zero"));
    }

    self->scale = args[ARG_scale].u_bool;
    if (self->scale) {
        if (((args[ARG_height].u_int % 3) != 0) || ((args[ARG_width].u_int % 3) != 0)) {
            mp_raise_ValueError(MP_ERROR_TEXT("Scale dimensions must divide by 3"));
        }

        self->scale_width = args[ARG_width].u_int / 3;
        self->scale_height = args[ARG_height].u_int / 3;
    } else {
        self->scale_width = args[ARG_width].u_int;
        self->scale_height = args[ARG_height].u_int;
    }

    self->auto_gamma = args[ARG_gamma].u_bool;

    mp_obj_t framebuffer = args[ARG_framebuffer].u_obj;
    if (framebuffer == mp_const_none) {
        int width = args[ARG_width].u_int;
        int height = args[ARG_height].u_int;
        int bufsize = 4 * width * height;
        framebuffer = mp_obj_new_bytearray_of_zeros(bufsize);
    }

    common_hal_is31fl3741_framebuffer_construct(self,
        args[ARG_width].u_int,
        args[ARG_height].u_int,
        framebuffer,
        args[ARG_is31].u_obj,
        args[ARG_mapping].u_obj
        );

    return MP_OBJ_FROM_PTR(self);
}

//|     def deinit(self) -> None:
//|         """Free the resources associated with this
//|         IS31FL3741 instance.  After deinitialization, no further operations
//|         may be performed."""
//|         ...
static mp_obj_t is31fl3741_framebuffer_deinit(mp_obj_t self_in) {
    is31fl3741_framebuffer_obj_t *self = (is31fl3741_framebuffer_obj_t *)self_in;
    common_hal_is31fl3741_framebuffer_deinit(self);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(is31fl3741_framebuffer_deinit_obj, is31fl3741_framebuffer_deinit);

static void check_for_deinit(is31fl3741_framebuffer_obj_t *self) {
    if (self->framebuffer == NULL) {
        raise_deinited_error();
    }
}

//|     brightness: float
//|     """In the current implementation, 0.0 turns the display off entirely
//|     and any other value up to 1.0 turns the display on fully."""
static mp_obj_t is31fl3741_framebuffer_get_brightness(mp_obj_t self_in) {
    is31fl3741_framebuffer_obj_t *self = (is31fl3741_framebuffer_obj_t *)self_in;
    check_for_deinit(self);
    uint8_t current = common_hal_is31fl3741_get_current(self->is31fl3741);

    float brightness = (float)current / (float)0xFF;
    return mp_obj_new_float(brightness);
}
MP_DEFINE_CONST_FUN_OBJ_1(is31fl3741_framebuffer_get_brightness_obj, is31fl3741_framebuffer_get_brightness);

static mp_obj_t is31fl3741_framebuffer_set_brightness(mp_obj_t self_in, mp_obj_t value_in) {
    is31fl3741_framebuffer_obj_t *self = (is31fl3741_framebuffer_obj_t *)self_in;
    check_for_deinit(self);
    mp_float_t brightness = mp_obj_get_float(value_in);
    if (brightness < 0.0f || brightness > 1.0f) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("%q must be %d-%d"), MP_QSTR_brightness, 0, 1);
    }

    uint8_t current = (uint8_t)(brightness * 0xFF);
    common_hal_is31fl3741_set_current(self->is31fl3741, current);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(is31fl3741_framebuffer_set_brightness_obj, is31fl3741_framebuffer_set_brightness);

MP_PROPERTY_GETSET(is31fl3741_framebuffer_brightness_obj,
    (mp_obj_t)&is31fl3741_framebuffer_get_brightness_obj,
    (mp_obj_t)&is31fl3741_framebuffer_set_brightness_obj);

//|     def refresh(self) -> None:
//|         """Transmits the color data in the buffer to the pixels so that
//|         they are shown."""
//|         ...
static mp_obj_t is31fl3741_framebuffer_refresh(mp_obj_t self_in) {
    is31fl3741_framebuffer_obj_t *self = (is31fl3741_framebuffer_obj_t *)self_in;
    check_for_deinit(self);
    common_hal_is31fl3741_framebuffer_refresh(self, 0);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(is31fl3741_framebuffer_refresh_obj, is31fl3741_framebuffer_refresh);

//|     width: int
//|     """The width of the display, in pixels"""
static mp_obj_t is31fl3741_framebuffer_get_width(mp_obj_t self_in) {
    is31fl3741_framebuffer_obj_t *self = (is31fl3741_framebuffer_obj_t *)self_in;
    check_for_deinit(self);
    return MP_OBJ_NEW_SMALL_INT(common_hal_is31fl3741_framebuffer_get_width(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(is31fl3741_framebuffer_get_width_obj, is31fl3741_framebuffer_get_width);
MP_PROPERTY_GETTER(is31fl3741_framebuffer_width_obj,
    (mp_obj_t)&is31fl3741_framebuffer_get_width_obj);

//|     height: int
//|     """The height of the display, in pixels"""
//|
static mp_obj_t is31fl3741_framebuffer_get_height(mp_obj_t self_in) {
    is31fl3741_framebuffer_obj_t *self = (is31fl3741_framebuffer_obj_t *)self_in;
    check_for_deinit(self);
    return MP_OBJ_NEW_SMALL_INT(common_hal_is31fl3741_framebuffer_get_height(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(is31fl3741_framebuffer_get_height_obj, is31fl3741_framebuffer_get_height);
MP_PROPERTY_GETTER(is31fl3741_framebuffer_height_obj,
    (mp_obj_t)&is31fl3741_framebuffer_get_height_obj);

static const mp_rom_map_elem_t is31fl3741_framebuffer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&is31fl3741_framebuffer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_brightness), MP_ROM_PTR(&is31fl3741_framebuffer_brightness_obj) },
    { MP_ROM_QSTR(MP_QSTR_refresh), MP_ROM_PTR(&is31fl3741_framebuffer_refresh_obj) },
    { MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&is31fl3741_framebuffer_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_height), MP_ROM_PTR(&is31fl3741_framebuffer_height_obj) },
};
static MP_DEFINE_CONST_DICT(is31fl3741_framebuffer_locals_dict, is31fl3741_framebuffer_locals_dict_table);

static void is31fl3741_framebuffer_get_bufinfo(mp_obj_t self_in, mp_buffer_info_t *bufinfo) {
    is31fl3741_framebuffer_obj_t *self = (is31fl3741_framebuffer_obj_t *)self_in;
    check_for_deinit(self);

    *bufinfo = self->bufinfo;
}

static void is31fl3741_framebuffer_swapbuffers(mp_obj_t self_in, uint8_t *dirty_row_bitmap) {
    common_hal_is31fl3741_framebuffer_refresh(self_in, dirty_row_bitmap);
}

static void is31fl3741_framebuffer_deinit_proto(mp_obj_t self_in) {
    common_hal_is31fl3741_framebuffer_deinit(self_in);
}

static float is31fl3741_framebuffer_get_brightness_proto(mp_obj_t self_in) {
    return common_hal_is31fl3741_framebuffer_get_paused(self_in) ? 0.0f : 1.0f;
}

static bool is31fl3741_framebuffer_set_brightness_proto(mp_obj_t self_in, mp_float_t value) {
    common_hal_is31fl3741_framebuffer_set_paused(self_in, value <= 0);
    return true;
}

static int is31fl3741_framebuffer_get_width_proto(mp_obj_t self_in) {
    return common_hal_is31fl3741_framebuffer_get_width(self_in);
}

static int is31fl3741_framebuffer_get_height_proto(mp_obj_t self_in) {
    return common_hal_is31fl3741_framebuffer_get_height(self_in);
}

static int is31fl3741_framebuffer_get_color_depth_proto(mp_obj_t self_in) {
    // The way displayio works depth is used to calculate bytes
    // We use an uint32_t for color already so setting to 24 causes
    // more changes required
    return 32;
}

static int is31fl3741_framebuffer_get_bytes_per_cell_proto(mp_obj_t self_in) {
    return 1;
}

static int is31fl3741_framebuffer_get_native_frames_per_second_proto(mp_obj_t self_in) {
    return 60; // This was just chosen may vary based on LEDs used?
}

static const framebuffer_p_t is31fl3741_framebuffer_proto = {
    MP_PROTO_IMPLEMENT(MP_QSTR_protocol_framebuffer)
    .get_bufinfo = is31fl3741_framebuffer_get_bufinfo,
    .set_brightness = is31fl3741_framebuffer_set_brightness_proto,
    .get_brightness = is31fl3741_framebuffer_get_brightness_proto,
    .get_width = is31fl3741_framebuffer_get_width_proto,
    .get_height = is31fl3741_framebuffer_get_height_proto,
    .get_color_depth = is31fl3741_framebuffer_get_color_depth_proto,
    .get_bytes_per_cell = is31fl3741_framebuffer_get_bytes_per_cell_proto,
    .get_native_frames_per_second = is31fl3741_framebuffer_get_native_frames_per_second_proto,
    .swapbuffers = is31fl3741_framebuffer_swapbuffers,
    .deinit = is31fl3741_framebuffer_deinit_proto,
};

static mp_int_t is31fl3741_framebuffer_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    is31fl3741_framebuffer_obj_t *self = (is31fl3741_framebuffer_obj_t *)self_in;
    // a readonly framebuffer would be unusual but not impossible
    if ((flags & MP_BUFFER_WRITE) && !(self->bufinfo.typecode & MP_OBJ_ARRAY_TYPECODE_FLAG_RW)) {
        return 1;
    }
    *bufinfo = self->bufinfo;
    bufinfo->typecode = 'H';
    return 0;
}

MP_DEFINE_CONST_OBJ_TYPE(
    is31fl3741_framebuffer_type,
    MP_QSTR_is31fl3741,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    locals_dict, &is31fl3741_framebuffer_locals_dict,
    make_new, is31fl3741_framebuffer_make_new,
    buffer, is31fl3741_framebuffer_get_buffer,
    protocol, &is31fl3741_framebuffer_proto
    );
