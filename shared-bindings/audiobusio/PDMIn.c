// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "shared/runtime/context_manager_helpers.h"
#include "py/binary.h"
#include "py/mphal.h"
#include "py/objproperty.h"
#include "py/runtime.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/audiobusio/PDMIn.h"
#include "shared-bindings/util.h"

//| class PDMIn:
//|     """Record an input PDM audio stream"""
//|
//|     def __init__(
//|         self,
//|         clock_pin: microcontroller.Pin,
//|         data_pin: microcontroller.Pin,
//|         *,
//|         sample_rate: int = 16000,
//|         bit_depth: int = 8,
//|         mono: bool = True,
//|         oversample: int = 64,
//|         startup_delay: float = 0.11
//|     ) -> None:
//|         """Create a PDMIn object associated with the given pins. This allows you to
//|         record audio signals from the given pins. Individual ports may put further
//|         restrictions on the recording parameters. The overall sample rate is
//|         determined by `sample_rate` x ``oversample``, and the total must be 1MHz or
//|         higher, so `sample_rate` must be a minimum of 16000.
//|
//|         :param ~microcontroller.Pin clock_pin: The pin to output the clock to
//|         :param ~microcontroller.Pin data_pin: The pin to read the data from
//|         :param int sample_rate: Target sample_rate of the resulting samples. Check `sample_rate` for actual value.
//|           Minimum sample_rate is about 16000 Hz.
//|         :param int bit_depth: Final number of bits per sample. Must be divisible by 8
//|         :param bool mono: True when capturing a single channel of audio, captures two channels otherwise
//|         :param int oversample: Number of single bit samples to decimate into a final sample. Must be divisible by 8
//|         :param float startup_delay: seconds to wait after starting microphone clock
//|          to allow microphone to turn on. Most require only 0.01s; some require 0.1s. Longer is safer.
//|          Must be in range 0.0-1.0 seconds.
//|
//|         **Limitations:** On SAMD and RP2040, supports only 8 or 16 bit mono input, with 64x oversampling.
//|         On nRF52840, supports only 16 bit mono input at 16 kHz; oversampling is fixed at 64x. Not provided
//|         on nRF52833 for space reasons. Not available on Espressif.
//|
//|         For example, to record 8-bit unsigned samples to a buffer::
//|
//|           import audiobusio
//|           import board
//|
//|           # Prep a buffer to record into
//|           b = bytearray(200)
//|           with audiobusio.PDMIn(board.MICROPHONE_CLOCK, board.MICROPHONE_DATA, sample_rate=16000) as mic:
//|               mic.record(b, len(b))
//|
//|         To record 16-bit unsigned samples to a buffer::
//|
//|           import audiobusio
//|           import board
//|
//|           # Prep a buffer to record into.
//|           b = array.array("H", [0] * 200)
//|           with audiobusio.PDMIn(board.MICROPHONE_CLOCK, board.MICROPHONE_DATA, sample_rate=16000, bit_depth=16) as mic:
//|               mic.record(b, len(b))
//|         """
//|     ...
static mp_obj_t audiobusio_pdmin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    #if !CIRCUITPY_AUDIOBUSIO_PDMIN
    mp_raise_NotImplementedError_varg(MP_ERROR_TEXT("%q"), MP_QSTR_PDMIn);
    #else
    enum { ARG_clock_pin, ARG_data_pin, ARG_sample_rate, ARG_bit_depth, ARG_mono, ARG_oversample, ARG_startup_delay };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_clock_pin,     MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_data_pin,      MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_sample_rate,   MP_ARG_KW_ONLY | MP_ARG_INT,  {.u_int = 16000} },
        { MP_QSTR_bit_depth,     MP_ARG_KW_ONLY | MP_ARG_INT,  {.u_int = 8} },
        { MP_QSTR_mono,          MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = true} },
        { MP_QSTR_oversample,    MP_ARG_KW_ONLY | MP_ARG_INT,  {.u_int = 64} },
        { MP_QSTR_startup_delay, MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj = MP_OBJ_NULL} },
    };
    // Default microphone startup delay is 110msecs. Have seen mics that need 100 msecs plus a bit.
    static const float STARTUP_DELAY_DEFAULT = 0.110F;

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    const mcu_pin_obj_t *clock_pin = validate_obj_is_free_pin(args[ARG_clock_pin].u_obj, MP_QSTR_clock_pin);
    const mcu_pin_obj_t *data_pin = validate_obj_is_free_pin(args[ARG_data_pin].u_obj, MP_QSTR_data_pin);

    // create PDMIn object from the given pin
    audiobusio_pdmin_obj_t *self = mp_obj_malloc(audiobusio_pdmin_obj_t, &audiobusio_pdmin_type);

    uint32_t sample_rate = args[ARG_sample_rate].u_int;
    uint8_t bit_depth = args[ARG_bit_depth].u_int;
    if (bit_depth % 8 != 0) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("%q must be multiple of 8."), MP_QSTR_bit_depth);
    }
    uint8_t oversample = args[ARG_oversample].u_int;
    if (oversample % 8 != 0) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("%q must be multiple of 8."), MP_QSTR_oversample);
    }
    bool mono = args[ARG_mono].u_bool;

    mp_float_t startup_delay = (args[ARG_startup_delay].u_obj == MP_OBJ_NULL)
        ? (mp_float_t)STARTUP_DELAY_DEFAULT
        : mp_obj_get_float(args[ARG_startup_delay].u_obj);
    mp_arg_validate_float_range(startup_delay, 0.0f, 1.0f, MP_QSTR_startup_delay);

    common_hal_audiobusio_pdmin_construct(self, clock_pin, data_pin, sample_rate,
        bit_depth, mono, oversample);

    // Wait for the microphone to start up. Some start in 10 msecs; some take as much as 100 msecs.
    mp_hal_delay_ms(startup_delay * 1000);

    return MP_OBJ_FROM_PTR(self);
    #endif
}

#if CIRCUITPY_AUDIOBUSIO_PDMIN
//|     def deinit(self) -> None:
//|         """Deinitialises the PDMIn and releases any hardware resources for reuse."""
//|         ...
static mp_obj_t audiobusio_pdmin_deinit(mp_obj_t self_in) {
    audiobusio_pdmin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_audiobusio_pdmin_deinit(self);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(audiobusio_pdmin_deinit_obj, audiobusio_pdmin_deinit);

static void check_for_deinit(audiobusio_pdmin_obj_t *self) {
    if (common_hal_audiobusio_pdmin_deinited(self)) {
        raise_deinited_error();
    }
}
//|     def __enter__(self) -> PDMIn:
//|         """No-op used by Context Managers."""
//|         ...
//  Provided by context manager helper.

//|     def __exit__(self) -> None:
//|         """Automatically deinitializes the hardware when exiting a context."""
//|         ...
static mp_obj_t audiobusio_pdmin_obj___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    common_hal_audiobusio_pdmin_deinit(args[0]);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(audiobusio_pdmin___exit___obj, 4, 4, audiobusio_pdmin_obj___exit__);


//|     def record(self, destination: WriteableBuffer, destination_length: int) -> None:
//|         """Records destination_length bytes of samples to destination. This is
//|         blocking.
//|
//|         An IOError may be raised when the destination is too slow to record the
//|         audio at the given rate. For internal flash, writing all 1s to the file
//|         before recording is recommended to speed up writes.
//|
//|         :return: The number of samples recorded. If this is less than ``destination_length``,
//|           some samples were missed due to processing time."""
//|         ...
static mp_obj_t audiobusio_pdmin_obj_record(mp_obj_t self_obj, mp_obj_t destination, mp_obj_t destination_length) {
    audiobusio_pdmin_obj_t *self = MP_OBJ_TO_PTR(self_obj);
    check_for_deinit(self);
    uint32_t length = mp_arg_validate_type_int(destination_length, MP_QSTR_length);
    mp_arg_validate_length_min(length, 0, MP_QSTR_length);

    mp_buffer_info_t bufinfo;
    if (mp_obj_is_type(destination, &mp_type_fileio)) {
        mp_raise_NotImplementedError(MP_ERROR_TEXT("Cannot record to a file"));
    } else if (mp_get_buffer(destination, &bufinfo, MP_BUFFER_WRITE)) {
        if (bufinfo.len / mp_binary_get_size('@', bufinfo.typecode, NULL) < length) {
            mp_raise_ValueError(MP_ERROR_TEXT("Destination capacity is smaller than destination_length."));
        }
        uint8_t bit_depth = common_hal_audiobusio_pdmin_get_bit_depth(self);
        if (bufinfo.typecode != 'H' && bit_depth == 16) {
            mp_raise_ValueError(MP_ERROR_TEXT("destination buffer must be an array of type 'H' for bit_depth = 16"));
        } else if (bufinfo.typecode != 'B' && bufinfo.typecode != BYTEARRAY_TYPECODE && bit_depth == 8) {
            mp_raise_ValueError(MP_ERROR_TEXT("destination buffer must be a bytearray or array of type 'B' for bit_depth = 8"));
        }
        // length is the buffer length in slots, not bytes.
        uint32_t length_written =
            common_hal_audiobusio_pdmin_record_to_buffer(self, bufinfo.buf, length);
        return MP_OBJ_NEW_SMALL_INT(length_written);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(audiobusio_pdmin_record_obj, audiobusio_pdmin_obj_record);

//|     sample_rate: int
//|     """The actual sample_rate of the recording. This may not match the constructed
//|     sample rate due to internal clock limitations."""
//|
static mp_obj_t audiobusio_pdmin_obj_get_sample_rate(mp_obj_t self_in) {
    audiobusio_pdmin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);
    return MP_OBJ_NEW_SMALL_INT(common_hal_audiobusio_pdmin_get_sample_rate(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(audiobusio_pdmin_get_sample_rate_obj, audiobusio_pdmin_obj_get_sample_rate);

MP_PROPERTY_GETTER(audiobusio_pdmin_sample_rate_obj,
    (mp_obj_t)&audiobusio_pdmin_get_sample_rate_obj);

static const mp_rom_map_elem_t audiobusio_pdmin_locals_dict_table[] = {
    // Methods
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&audiobusio_pdmin_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&default___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&audiobusio_pdmin___exit___obj) },
    { MP_ROM_QSTR(MP_QSTR_record), MP_ROM_PTR(&audiobusio_pdmin_record_obj) },
    { MP_ROM_QSTR(MP_QSTR_sample_rate), MP_ROM_PTR(&audiobusio_pdmin_sample_rate_obj) }
};
static MP_DEFINE_CONST_DICT(audiobusio_pdmin_locals_dict, audiobusio_pdmin_locals_dict_table);
#endif

MP_DEFINE_CONST_OBJ_TYPE(
    audiobusio_pdmin_type,
    MP_QSTR_PDMIn,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    make_new, audiobusio_pdmin_make_new
    #if CIRCUITPY_AUDIOBUSIO_PDMIN
    , locals_dict, &audiobusio_pdmin_locals_dict
    #endif
    );
