// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Radomir Dopieralski
//
// SPDX-License-Identifier: MIT

#include "__init__.h"
#include "py/mperrno.h"
#include "py/runtime.h"
#include "shared-bindings/busio/SPI.h"
#include "shared-bindings/busdisplay/BusDisplay.h"
#include "shared-module/_stage/__init__.h"
#include "shared-module/displayio/display_core.h"
#include "Layer.h"
#include "Text.h"

//| """C-level helpers for animation of sprites on a stage
//|
//| The `_stage` module contains native code to speed-up the ```stage`` Library
//| <https://github.com/python-ugame/circuitpython-stage>`_."""
//|
//| def render(
//|     x0: int,
//|     y0: int,
//|     x1: int,
//|     y1: int,
//|     layers: List[Layer],
//|     buffer: WriteableBuffer,
//|     display: busdisplay.BusDisplay,
//|     scale: int,
//|     background: int,
//| ) -> None:
//|     """Render and send to the display a fragment of the screen.
//|
//|     :param int x0: Left edge of the fragment.
//|     :param int y0: Top edge of the fragment.
//|     :param int x1: Right edge of the fragment.
//|     :param int y1: Bottom edge of the fragment.
//|     :param layers: A list of the :py:class:`~_stage.Layer` objects.
//|     :type layers: list[Layer]
//|     :param ~circuitpython_typing.WriteableBuffer buffer: A buffer to use for rendering.
//|     :param ~busdisplay.BusDisplay display: The display to use.
//|     :param int scale: How many times should the image be scaled up.
//|     :param int background: What color to display when nothing is there.
//|
//|     There are also no sanity checks, outside of the basic overflow
//|     checking. The caller is responsible for making the passed parameters
//|     valid.
//|
//|     This function is intended for internal use in the ``stage`` library
//|     and all the necessary checks are performed there."""
//|
static mp_obj_t stage_render(size_t n_args, const mp_obj_t *args) {
    uint16_t x0 = mp_obj_get_int(args[0]);
    uint16_t y0 = mp_obj_get_int(args[1]);
    uint16_t x1 = mp_obj_get_int(args[2]);
    uint16_t y1 = mp_obj_get_int(args[3]);

    size_t layers_size = 0;
    mp_obj_t *layers;
    mp_obj_get_array(args[4], &layers_size, &layers);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[5], &bufinfo, MP_BUFFER_WRITE);
    uint16_t *buffer = bufinfo.buf;
    size_t buffer_size = bufinfo.len / 2; // 16-bit indexing

    mp_obj_t native_display = mp_obj_cast_to_native_base(args[6],
        &busdisplay_busdisplay_type);
    if (!mp_obj_is_type(native_display, &busdisplay_busdisplay_type)) {
        mp_raise_TypeError(MP_ERROR_TEXT("argument num/types mismatch"));
    }
    busdisplay_busdisplay_obj_t *display = MP_OBJ_TO_PTR(native_display);
    uint8_t scale = mp_obj_get_int(args[7]);
    int16_t vx = mp_obj_get_int(args[8]);
    int16_t vy = mp_obj_get_int(args[9]);
    uint16_t background = 0;

    render_stage(x0, y0, x1, y1, vx, vy, layers, layers_size,
        buffer, buffer_size, display, scale, background);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(stage_render_obj, 10, 10, stage_render);


static const mp_rom_map_elem_t stage_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR__stage) },
    { MP_ROM_QSTR(MP_QSTR_Layer), MP_ROM_PTR(&mp_type_layer) },
    { MP_ROM_QSTR(MP_QSTR_Text), MP_ROM_PTR(&mp_type_text) },
    { MP_ROM_QSTR(MP_QSTR_render), MP_ROM_PTR(&stage_render_obj) },
};

static MP_DEFINE_CONST_DICT(stage_module_globals, stage_module_globals_table);

const mp_obj_module_t stage_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&stage_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR__stage, stage_module);
