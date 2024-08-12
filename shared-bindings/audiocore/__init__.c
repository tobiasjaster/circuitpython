// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "py/obj.h"
#include "py/gc.h"
#include "py/runtime.h"

#include "shared-bindings/audiocore/__init__.h"
#include "shared-bindings/audiocore/RawSample.h"
#include "shared-bindings/audiocore/WaveFile.h"
// #include "shared-bindings/audiomixer/Mixer.h"

//| """Support for audio samples"""

#if CIRCUITPY_AUDIOCORE_DEBUG
// (no docstrings so that the debug functions are not shown on docs.circuitpython.org)
static mp_obj_t audiocore_get_buffer(mp_obj_t sample_in) {
    uint8_t *buffer = NULL;
    uint32_t buffer_length = 0;
    audioio_get_buffer_result_t gbr = audiosample_get_buffer(sample_in, false, 0, &buffer, &buffer_length);

    mp_obj_t result[2] = {mp_obj_new_int_from_uint(gbr), mp_const_none};

    if (gbr != GET_BUFFER_ERROR) {
        bool single_buffer, samples_signed;
        uint32_t max_buffer_length;
        uint8_t spacing;

        uint8_t bits_per_sample = audiosample_bits_per_sample(sample_in);
        audiosample_get_buffer_structure(sample_in, false, &single_buffer, &samples_signed, &max_buffer_length, &spacing);
        // copies the data because the gc semantics of get_buffer are unclear
        void *result_buf = m_malloc(buffer_length);
        memcpy(result_buf, buffer, buffer_length);
        char typecode =
            (bits_per_sample == 8 && samples_signed) ? 'b' :
            (bits_per_sample == 8 && !samples_signed) ? 'B' :
            (bits_per_sample == 16 && samples_signed) ? 'h' :
            (bits_per_sample == 16 && !samples_signed) ? 'H' :
            'b';
        size_t nitems = buffer_length / (bits_per_sample / 8);
        result[1] = mp_obj_new_memoryview(typecode, nitems, result_buf);
    }

    return mp_obj_new_tuple(2, result);
}
static MP_DEFINE_CONST_FUN_OBJ_1(audiocore_get_buffer_obj, audiocore_get_buffer);

static mp_obj_t audiocore_get_structure(mp_obj_t sample_in) {
    bool single_buffer, samples_signed;
    uint32_t max_buffer_length;
    uint8_t spacing;

    audiosample_get_buffer_structure(sample_in, false, &single_buffer, &samples_signed, &max_buffer_length, &spacing);
    mp_obj_t result[4] = {
        mp_obj_new_int_from_uint(single_buffer),
        mp_obj_new_int_from_uint(samples_signed),
        mp_obj_new_int_from_uint(max_buffer_length),
        mp_obj_new_int_from_uint(spacing),
    };
    return mp_obj_new_tuple(4, result);
}
static MP_DEFINE_CONST_FUN_OBJ_1(audiocore_get_structure_obj, audiocore_get_structure);

static mp_obj_t audiocore_reset_buffer(mp_obj_t sample_in) {
    audiosample_reset_buffer(sample_in, false, 0);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(audiocore_reset_buffer_obj, audiocore_reset_buffer);

#endif

static const mp_rom_map_elem_t audiocore_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_audiocore) },
    { MP_ROM_QSTR(MP_QSTR_RawSample), MP_ROM_PTR(&audioio_rawsample_type) },
    { MP_ROM_QSTR(MP_QSTR_WaveFile), MP_ROM_PTR(&audioio_wavefile_type) },
    #if CIRCUITPY_AUDIOCORE_DEBUG
    { MP_ROM_QSTR(MP_QSTR_get_buffer), MP_ROM_PTR(&audiocore_get_buffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_buffer), MP_ROM_PTR(&audiocore_reset_buffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_structure), MP_ROM_PTR(&audiocore_get_structure_obj) },
    #endif
};

static MP_DEFINE_CONST_DICT(audiocore_module_globals, audiocore_module_globals_table);

const mp_obj_module_t audiocore_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&audiocore_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_audiocore, audiocore_module);
