// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 Jeff Epler for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "py/obj.h"
#include "py/runtime.h"

#include "shared-bindings/bitops/__init__.h"

//| """Routines for low-level manipulation of binary data"""
//|

//| def bit_transpose(
//|     input: ReadableBuffer, output: WriteableBuffer, width: int = 8
//| ) -> WriteableBuffer:
//|     """ "Transpose" a buffer by assembling each output byte with bits taken from each of ``width`` different input bytes.
//|
//|     This can be useful to convert a sequence of pixel values into a single
//|     stream of bytes suitable for sending via a parallel conversion method.
//|
//|     The number of bytes in the input buffer must be a multiple of the width,
//|     and the width can be any value from 2 to 8.  If the width is fewer than 8,
//|     then the remaining (less significant) bits of the output are set to zero.
//|
//|     Let ``stride = len(input)//width``.  Then the first byte is made out of the
//|     most significant bits of ``[input[0], input[stride], input[2*stride], ...]``.
//|     The second byte is made out of the second bits, and so on until the 8th output
//|     byte which is made of the first bits of ``input[1], input[1+stride,
//|     input[2*stride], ...]``.
//|
//|     The required output buffer size is ``len(input) * 8  // width``.
//|
//|     Returns the output buffer."""
//|     ...
//|

static mp_obj_t bit_transpose(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_input, ARG_output, ARG_width };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_input, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_output, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_width, MP_ARG_INT, { .u_int = 8 } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_int_t width = mp_arg_validate_int_range(args[ARG_width].u_int, 2, 8, MP_QSTR_width);

    mp_buffer_info_t input_bufinfo;
    mp_get_buffer_raise(args[ARG_input].u_obj, &input_bufinfo, MP_BUFFER_READ);
    int inlen = input_bufinfo.len;
    if (inlen % width != 0) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("Input buffer length (%d) must be a multiple of the strand count (%d)"), inlen, width);
    }

    mp_buffer_info_t output_bufinfo;
    mp_get_buffer_raise(args[ARG_output].u_obj, &output_bufinfo, MP_BUFFER_WRITE);
    int avail = output_bufinfo.len;
    int outlen = 8 * (inlen / width);

    mp_arg_validate_length_min(avail, outlen, MP_QSTR_output);

    common_hal_bitops_bit_transpose(output_bufinfo.buf, input_bufinfo.buf, inlen, width);
    return args[ARG_output].u_obj;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(bitops_bit_transpose_obj, 0, bit_transpose);

static const mp_rom_map_elem_t bitops_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_bitops) },
    { MP_ROM_QSTR(MP_QSTR_bit_transpose), MP_ROM_PTR(&bitops_bit_transpose_obj) },
};

static MP_DEFINE_CONST_DICT(bitops_module_globals, bitops_module_globals_table);

const mp_obj_module_t bitops_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&bitops_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_bitops, bitops_module);
