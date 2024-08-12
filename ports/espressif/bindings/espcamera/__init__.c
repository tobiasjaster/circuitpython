// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2022 Jeff Epler for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"

#include "bindings/espidf/__init__.h"
#include "bindings/espcamera/__init__.h"
#include "bindings/espcamera/Camera.h"

#include "esp_camera.h"
#include "sensor.h"

//| """Wrapper for the espcamera library
//|
//| This library enables access to any camera sensor supported by the library,
//| including OV5640 and OV2640.
//|
//| .. seealso::
//|
//|     Non-Espressif microcontrollers use the `imagecapture` module together with wrapper libraries such as `adafruit_ov5640 <https://circuitpython.readthedocs.io/projects/ov5640/en/latest/>`_.
//|
//| """
//|

//| class GrabMode:
//|     """Controls when a new frame is grabbed."""
//|
//|     WHEN_EMPTY: GrabMode
//|     """Fills buffers when they are empty. Less resources but first ``fb_count`` frames might be old"""
//|
//|     LATEST: GrabMode
//|     """Except when 1 frame buffer is used, queue will always contain the last ``fb_count`` frames"""
//|

MAKE_ENUM_VALUE(espcamera_grab_mode_type, grab_mode, WHEN_EMPTY, CAMERA_GRAB_WHEN_EMPTY);
MAKE_ENUM_VALUE(espcamera_grab_mode_type, grab_mode, LATEST, CAMERA_GRAB_LATEST);

MAKE_ENUM_MAP(espcamera_grab_mode) {
    MAKE_ENUM_MAP_ENTRY(grab_mode, WHEN_EMPTY),
    MAKE_ENUM_MAP_ENTRY(grab_mode, LATEST),
};

static MP_DEFINE_CONST_DICT(espcamera_grab_mode_locals_dict, espcamera_grab_mode_locals_table);
MAKE_PRINTER(espcamera, espcamera_grab_mode);
MAKE_ENUM_TYPE(espcamera, GrabMode, espcamera_grab_mode);

camera_grab_mode_t validate_grab_mode(mp_obj_t obj, qstr arg_name) {
    return cp_enum_value(&espcamera_grab_mode_type, obj, arg_name);
}

//| class PixelFormat:
//|     """Format of data in the captured frames"""
//|
//|     RGB565: PixelFormat
//|     """A 16-bit format with 5 bits of Red and Blue and 6 bits of Green"""
//|
//|     GRAYSCALE: PixelFormat
//|     """An 8-bit format with 8-bits of luminance"""
//|
//|     JPEG: PixelFormat
//|     """A compressed format"""
//|

MAKE_ENUM_VALUE(espcamera_pixel_format_type, pixel_format, RGB565, PIXFORMAT_RGB565);
MAKE_ENUM_VALUE(espcamera_pixel_format_type, pixel_format, GRAYSCALE, PIXFORMAT_GRAYSCALE);
MAKE_ENUM_VALUE(espcamera_pixel_format_type, pixel_format, JPEG, PIXFORMAT_JPEG);

MAKE_ENUM_MAP(espcamera_pixel_format) {
    MAKE_ENUM_MAP_ENTRY(pixel_format, RGB565),
    MAKE_ENUM_MAP_ENTRY(pixel_format, GRAYSCALE),
    MAKE_ENUM_MAP_ENTRY(pixel_format, JPEG),
};

static MP_DEFINE_CONST_DICT(espcamera_pixel_format_locals_dict, espcamera_pixel_format_locals_table);
MAKE_PRINTER(espcamera, espcamera_pixel_format);
MAKE_ENUM_TYPE(espcamera, PixelFormat, espcamera_pixel_format);

pixformat_t validate_pixel_format(mp_obj_t obj, qstr arg_name) {
    return cp_enum_value(&espcamera_pixel_format_type, obj, arg_name);
}

//| class FrameSize:
//|     """The pixel size of the captured frames"""
//|
//|     R96X96: FrameSize
//|     """96x96"""
//|
//|     QQVGA: FrameSize
//|     """160x120"""
//|
//|     QCIF: FrameSize
//|     """176x144"""
//|
//|     HQVGA: FrameSize
//|     """240x176"""
//|
//|     R240X240: FrameSize
//|     """240x240"""
//|
//|     QVGA: FrameSize
//|     """320x240 """
//|
//|     CIF: FrameSize
//|     """400x296"""
//|
//|     HVGA: FrameSize
//|     """480x320"""
//|
//|     VGA: FrameSize
//|     """640x480"""
//|
//|     SVGA: FrameSize
//|     """800x600"""
//|
//|     XGA: FrameSize
//|     """1024x768"""
//|
//|     HD: FrameSize
//|     """1280x720"""
//|
//|     SXGA: FrameSize
//|     """1280x1024"""
//|
//|     UXGA: FrameSize
//|     """1600x1200"""
//|
//|     FHD: FrameSize
//|     """1920x1080"""
//|
//|     P_HD: FrameSize
//|     """ 720x1280"""
//|
//|     P_3MP: FrameSize
//|     """ 864x1536"""
//|
//|     QXGA: FrameSize
//|     """2048x1536"""
//|
//|     QHD: FrameSize
//|     """2560x1440"""
//|
//|     WQXGA: FrameSize
//|     """2560x1600"""
//|
//|     P_FHD: FrameSize
//|     """1080x1920"""
//|
//|     QSXGA: FrameSize
//|     """2560x1920"""
//|

MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, R96X96, FRAMESIZE_96X96);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, R240X240, FRAMESIZE_240X240);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, QQVGA, FRAMESIZE_QQVGA);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, QCIF, FRAMESIZE_QCIF);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, HQVGA, FRAMESIZE_HQVGA);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, QVGA, FRAMESIZE_QVGA);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, CIF, FRAMESIZE_CIF);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, HVGA, FRAMESIZE_HVGA);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, VGA, FRAMESIZE_VGA);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, SVGA, FRAMESIZE_SVGA);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, XGA, FRAMESIZE_XGA);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, HD, FRAMESIZE_HD);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, SXGA, FRAMESIZE_SXGA);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, UXGA, FRAMESIZE_UXGA);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, FHD, FRAMESIZE_FHD);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, P_HD, FRAMESIZE_P_HD);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, P_3MP, FRAMESIZE_P_3MP);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, QXGA, FRAMESIZE_QXGA);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, QHD, FRAMESIZE_QHD);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, WQXGA, FRAMESIZE_WQXGA);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, P_FHD, FRAMESIZE_P_FHD);
MAKE_ENUM_VALUE(espcamera_frame_size_type, frame_size, QSXGA, FRAMESIZE_QSXGA);
MAKE_ENUM_MAP(espcamera_frame_size) {
    MAKE_ENUM_MAP_ENTRY(frame_size, R96X96),
    MAKE_ENUM_MAP_ENTRY(frame_size, R240X240),
    MAKE_ENUM_MAP_ENTRY(frame_size, QQVGA),
    MAKE_ENUM_MAP_ENTRY(frame_size, QCIF),
    MAKE_ENUM_MAP_ENTRY(frame_size, HQVGA),
    MAKE_ENUM_MAP_ENTRY(frame_size, QVGA),
    MAKE_ENUM_MAP_ENTRY(frame_size, CIF),
    MAKE_ENUM_MAP_ENTRY(frame_size, HVGA),
    MAKE_ENUM_MAP_ENTRY(frame_size, VGA),
    MAKE_ENUM_MAP_ENTRY(frame_size, SVGA),
    MAKE_ENUM_MAP_ENTRY(frame_size, XGA),
    MAKE_ENUM_MAP_ENTRY(frame_size, HD),
    MAKE_ENUM_MAP_ENTRY(frame_size, SXGA),
    MAKE_ENUM_MAP_ENTRY(frame_size, UXGA),
    MAKE_ENUM_MAP_ENTRY(frame_size, FHD),
    MAKE_ENUM_MAP_ENTRY(frame_size, P_HD),
    MAKE_ENUM_MAP_ENTRY(frame_size, P_3MP),
    MAKE_ENUM_MAP_ENTRY(frame_size, QXGA),
    MAKE_ENUM_MAP_ENTRY(frame_size, QHD),
    MAKE_ENUM_MAP_ENTRY(frame_size, WQXGA),
    MAKE_ENUM_MAP_ENTRY(frame_size, P_FHD),
    MAKE_ENUM_MAP_ENTRY(frame_size, QSXGA),
};

static MP_DEFINE_CONST_DICT(espcamera_frame_size_locals_dict, espcamera_frame_size_locals_table);
MAKE_PRINTER(espcamera, espcamera_frame_size);
MAKE_ENUM_TYPE(espcamera, FrameSize, espcamera_frame_size);

framesize_t validate_frame_size(mp_obj_t obj, qstr arg_name) {
    return cp_enum_value(&espcamera_frame_size_type, obj, arg_name);
}

//| class GainCeiling:
//|     """The maximum amount of gain applied to raw sensor data.
//|
//|     Higher values are useful in darker conditions, but increase image noise."""
//|
//|     GAIN_2X: GainCeiling
//|     GAIN_4X: GainCeiling
//|     GAIN_8X: GainCeiling
//|     GAIN_16X: GainCeiling
//|     GAIN_32X: GainCeiling
//|     GAIN_64X: GainCeiling
//|     GAIN_128X: GainCeiling
//|

MAKE_ENUM_VALUE(espcamera_gain_ceiling_type, gain_ceiling, GAIN_2X, GAINCEILING_2X);
MAKE_ENUM_VALUE(espcamera_gain_ceiling_type, gain_ceiling, GAIN_4X, GAINCEILING_4X);
MAKE_ENUM_VALUE(espcamera_gain_ceiling_type, gain_ceiling, GAIN_8X, GAINCEILING_8X);
MAKE_ENUM_VALUE(espcamera_gain_ceiling_type, gain_ceiling, GAIN_16X, GAINCEILING_16X);
MAKE_ENUM_VALUE(espcamera_gain_ceiling_type, gain_ceiling, GAIN_32X, GAINCEILING_32X);
MAKE_ENUM_VALUE(espcamera_gain_ceiling_type, gain_ceiling, GAIN_64X, GAINCEILING_64X);
MAKE_ENUM_VALUE(espcamera_gain_ceiling_type, gain_ceiling, GAIN_128X, GAINCEILING_128X);

MAKE_ENUM_MAP(espcamera_gain_ceiling) {
    MAKE_ENUM_MAP_ENTRY(gain_ceiling, GAIN_2X),
    MAKE_ENUM_MAP_ENTRY(gain_ceiling, GAIN_4X),
    MAKE_ENUM_MAP_ENTRY(gain_ceiling, GAIN_8X),
    MAKE_ENUM_MAP_ENTRY(gain_ceiling, GAIN_16X),
    MAKE_ENUM_MAP_ENTRY(gain_ceiling, GAIN_32X),
    MAKE_ENUM_MAP_ENTRY(gain_ceiling, GAIN_64X),
    MAKE_ENUM_MAP_ENTRY(gain_ceiling, GAIN_128X)
};

static MP_DEFINE_CONST_DICT(espcamera_gain_ceiling_locals_dict, espcamera_gain_ceiling_locals_table);
MAKE_PRINTER(espcamera, espcamera_gain_ceiling);
MAKE_ENUM_TYPE(espcamera, GainCeiling, espcamera_gain_ceiling);

gainceiling_t validate_gain_ceiling(mp_obj_t obj, qstr arg_name) {
    return cp_enum_value(&espcamera_gain_ceiling_type, obj, arg_name);
}

static const mp_rom_map_elem_t espcamera_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_espcamera) },
    { MP_ROM_QSTR(MP_QSTR_Camera), MP_ROM_PTR(&espcamera_camera_type), },
    { MP_ROM_QSTR(MP_QSTR_FrameSize), &espcamera_frame_size_type },
    { MP_ROM_QSTR(MP_QSTR_GainCeiling), &espcamera_gain_ceiling_type },
    { MP_ROM_QSTR(MP_QSTR_GrabMode), &espcamera_grab_mode_type },
    { MP_ROM_QSTR(MP_QSTR_PixelFormat), &espcamera_pixel_format_type },
};

static MP_DEFINE_CONST_DICT(espcamera_module_globals, espcamera_module_globals_table);

const mp_obj_module_t espcamera_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&espcamera_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_espcamera, espcamera_module);
