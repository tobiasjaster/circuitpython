// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 Artyom Skrobov
// SPDX-FileCopyrightText: Copyright (c) 2023 Jeff Epler for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "py/objproperty.h"
#include "py/runtime.h"
#include "shared-bindings/util.h"
#include "shared-bindings/synthio/__init__.h"
#include "shared-bindings/synthio/Note.h"
#include "shared-module/synthio/Note.h"

static const mp_arg_t note_properties[] = {
    { MP_QSTR_frequency, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = NULL } },
    { MP_QSTR_panning, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_obj = MP_ROM_INT(0) } },
    { MP_QSTR_amplitude, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_obj = MP_ROM_INT(1) } },
    { MP_QSTR_bend, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_obj = MP_ROM_INT(0) } },
    { MP_QSTR_waveform, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_obj = MP_ROM_NONE } },
    { MP_QSTR_waveform_loop_start, MP_ARG_OBJ, {.u_obj = MP_ROM_INT(0) } },
    { MP_QSTR_waveform_loop_end, MP_ARG_OBJ, {.u_obj = MP_ROM_INT(SYNTHIO_WAVEFORM_SIZE) } },
    { MP_QSTR_envelope, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_obj = MP_ROM_NONE } },
    { MP_QSTR_filter, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_obj = MP_ROM_NONE } },
    { MP_QSTR_ring_frequency, MP_ARG_OBJ, {.u_obj = MP_ROM_INT(0) } },
    { MP_QSTR_ring_bend, MP_ARG_OBJ, {.u_obj = MP_ROM_INT(0) } },
    { MP_QSTR_ring_waveform, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_obj = MP_ROM_NONE } },
    { MP_QSTR_ring_waveform_loop_start, MP_ARG_OBJ, {.u_obj = MP_ROM_INT(0) } },
    { MP_QSTR_ring_waveform_loop_end, MP_ARG_OBJ, {.u_obj = MP_ROM_INT(SYNTHIO_WAVEFORM_SIZE) } },
};
//| class Note:
//|     def __init__(
//|         self,
//|         *,
//|         frequency: float,
//|         panning: BlockInput = 0.0,
//|         waveform: Optional[ReadableBuffer] = None,
//|         waveform_loop_start: int = 0,
//|         waveform_loop_end: int = waveform_max_length,
//|         envelope: Optional[Envelope] = None,
//|         amplitude: BlockInput = 0.0,
//|         bend: BlockInput = 0.0,
//|         filter: Optional[Biquad] = None,
//|         ring_frequency: float = 0.0,
//|         ring_bend: float = 0.0,
//|         ring_waveform: Optional[ReadableBuffer] = None,
//|         ring_waveform_loop_start: int = 0,
//|         ring_waveform_loop_end: int = waveform_max_length,
//|     ) -> None:
//|         """Construct a Note object, with a frequency in Hz, and optional panning, waveform, envelope, tremolo (volume change) and bend (frequency change).
//|
//|         If waveform or envelope are `None` the synthesizer object's default waveform or envelope are used.
//|
//|         If the same Note object is played on multiple Synthesizer objects, the result is undefined.
//|         """
static mp_obj_t synthio_note_make_new(const mp_obj_type_t *type_in, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    mp_arg_val_t args[MP_ARRAY_SIZE(note_properties)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(note_properties), note_properties, args);

    synthio_note_obj_t *self = mp_obj_malloc(synthio_note_obj_t, &synthio_note_type);

    mp_obj_t result = MP_OBJ_FROM_PTR(self);
    properties_construct_helper(result, note_properties, args, MP_ARRAY_SIZE(note_properties));

    return result;
};

//|     frequency: float
//|     """The base frequency of the note, in Hz."""
static mp_obj_t synthio_note_get_frequency(mp_obj_t self_in) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_float(common_hal_synthio_note_get_frequency(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(synthio_note_get_frequency_obj, synthio_note_get_frequency);

static mp_obj_t synthio_note_set_frequency(mp_obj_t self_in, mp_obj_t arg) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_synthio_note_set_frequency(self, mp_obj_get_float(arg));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(synthio_note_set_frequency_obj, synthio_note_set_frequency);
MP_PROPERTY_GETSET(synthio_note_frequency_obj,
    (mp_obj_t)&synthio_note_get_frequency_obj,
    (mp_obj_t)&synthio_note_set_frequency_obj);

//|     filter: Optional[Biquad]
//|     """If not None, the output of this Note is filtered according to the provided coefficients.
//|
//|     Construct an appropriate filter by calling a filter-making method on the
//|     `Synthesizer` object where you plan to play the note, as filter coefficients depend
//|     on the sample rate"""
static mp_obj_t synthio_note_get_filter(mp_obj_t self_in) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return common_hal_synthio_note_get_filter_obj(self);
}
MP_DEFINE_CONST_FUN_OBJ_1(synthio_note_get_filter_obj, synthio_note_get_filter);

static mp_obj_t synthio_note_set_filter(mp_obj_t self_in, mp_obj_t arg) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_synthio_note_set_filter(self, arg);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(synthio_note_set_filter_obj, synthio_note_set_filter);
MP_PROPERTY_GETSET(synthio_note_filter_obj,
    (mp_obj_t)&synthio_note_get_filter_obj,
    (mp_obj_t)&synthio_note_set_filter_obj);

//|     panning: BlockInput
//|     """Defines the channel(s) in which the note appears.
//|
//|     -1 is left channel only, 0 is both channels, and 1 is right channel.
//|     For fractional values, the note plays at full amplitude in one channel
//|     and partial amplitude in the other channel. For instance -.5 plays at full
//|     amplitude in the left channel and 1/2 amplitude in the right channel."""
static mp_obj_t synthio_note_get_panning(mp_obj_t self_in) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return common_hal_synthio_note_get_panning(self);
}
MP_DEFINE_CONST_FUN_OBJ_1(synthio_note_get_panning_obj, synthio_note_get_panning);

static mp_obj_t synthio_note_set_panning(mp_obj_t self_in, mp_obj_t arg) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_synthio_note_set_panning(self, arg);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(synthio_note_set_panning_obj, synthio_note_set_panning);
MP_PROPERTY_GETSET(synthio_note_panning_obj,
    (mp_obj_t)&synthio_note_get_panning_obj,
    (mp_obj_t)&synthio_note_set_panning_obj);


//|     amplitude: BlockInput
//|     """The relative amplitude of the note, from 0 to 1
//|
//|     An amplitude of 0 makes the note inaudible. It is combined multiplicatively with
//|     the value from the note's envelope.
//|
//|     To achieve a tremolo effect, attach an LFO here."""
static mp_obj_t synthio_note_get_amplitude(mp_obj_t self_in) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return common_hal_synthio_note_get_amplitude(self);
}
MP_DEFINE_CONST_FUN_OBJ_1(synthio_note_get_amplitude_obj, synthio_note_get_amplitude);

static mp_obj_t synthio_note_set_amplitude(mp_obj_t self_in, mp_obj_t arg) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_synthio_note_set_amplitude(self, arg);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(synthio_note_set_amplitude_obj, synthio_note_set_amplitude);
MP_PROPERTY_GETSET(synthio_note_amplitude_obj,
    (mp_obj_t)&synthio_note_get_amplitude_obj,
    (mp_obj_t)&synthio_note_set_amplitude_obj);

//|
//|     bend: BlockInput
//|     """The pitch bend depth of the note, from -12 to +12
//|
//|     A depth of 0 plays the programmed frequency. A depth of 1 corresponds to a bend of 1
//|     octave.  A depth of (1/12) = 0.0833 corresponds to a bend of 1 semitone,
//|     and a depth of .00833 corresponds to one musical cent.
//|
//|     To achieve a vibrato or sweep effect, attach an LFO here.
//|     """
static mp_obj_t synthio_note_get_bend(mp_obj_t self_in) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return common_hal_synthio_note_get_bend(self);
}
MP_DEFINE_CONST_FUN_OBJ_1(synthio_note_get_bend_obj, synthio_note_get_bend);

static mp_obj_t synthio_note_set_bend(mp_obj_t self_in, mp_obj_t arg) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_synthio_note_set_bend(self, arg);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(synthio_note_set_bend_obj, synthio_note_set_bend);
MP_PROPERTY_GETSET(synthio_note_bend_obj,
    (mp_obj_t)&synthio_note_get_bend_obj,
    (mp_obj_t)&synthio_note_set_bend_obj);

//|     waveform: Optional[ReadableBuffer]
//|     """The waveform of this note. Setting the waveform to a buffer of a different size resets the note's phase."""
static mp_obj_t synthio_note_get_waveform(mp_obj_t self_in) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return common_hal_synthio_note_get_waveform_obj(self);
}
MP_DEFINE_CONST_FUN_OBJ_1(synthio_note_get_waveform_obj, synthio_note_get_waveform);

static mp_obj_t synthio_note_set_waveform(mp_obj_t self_in, mp_obj_t arg) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_synthio_note_set_waveform(self, arg);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(synthio_note_set_waveform_obj, synthio_note_set_waveform);
MP_PROPERTY_GETSET(synthio_note_waveform_obj,
    (mp_obj_t)&synthio_note_get_waveform_obj,
    (mp_obj_t)&synthio_note_set_waveform_obj);

//|     waveform_loop_start: int
//|     """The sample index of where to begin looping waveform data.
//|
//|     Values outside the range ``0`` to ``waveform_max_length-1`` (inclusive) are rejected with a `ValueError`.
//|
//|     Values greater than or equal to the actual waveform length are treated as 0."""
static mp_obj_t synthio_note_get_waveform_loop_start(mp_obj_t self_in) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(common_hal_synthio_note_get_waveform_loop_start(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(synthio_note_get_waveform_loop_start_obj, synthio_note_get_waveform_loop_start);

static mp_obj_t synthio_note_set_waveform_loop_start(mp_obj_t self_in, mp_obj_t arg) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_synthio_note_set_waveform_loop_start(self, mp_obj_get_int(arg));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(synthio_note_set_waveform_loop_start_obj, synthio_note_set_waveform_loop_start);
MP_PROPERTY_GETSET(synthio_note_waveform_loop_start_obj,
    (mp_obj_t)&synthio_note_get_waveform_loop_start_obj,
    (mp_obj_t)&synthio_note_set_waveform_loop_start_obj);

//|     waveform_loop_end: int
//|     """The sample index of where to end looping waveform data.
//|
//|     Values outside the range ``1`` to ``waveform_max_length`` (inclusive) are rejected with a `ValueError`.
//|
//|     If the value is greater than the actual waveform length, or less than or equal to the loop start, the loop will occur at the end of the waveform.
//|
//|     Use the `synthio.waveform_max_length` constant to set the loop point at the end of the wave form, no matter its length."""
//|
static mp_obj_t synthio_note_get_waveform_loop_end(mp_obj_t self_in) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(common_hal_synthio_note_get_waveform_loop_end(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(synthio_note_get_waveform_loop_end_obj, synthio_note_get_waveform_loop_end);

static mp_obj_t synthio_note_set_waveform_loop_end(mp_obj_t self_in, mp_obj_t arg) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_synthio_note_set_waveform_loop_end(self, mp_obj_get_int(arg));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(synthio_note_set_waveform_loop_end_obj, synthio_note_set_waveform_loop_end);
MP_PROPERTY_GETSET(synthio_note_waveform_loop_end_obj,
    (mp_obj_t)&synthio_note_get_waveform_loop_end_obj,
    (mp_obj_t)&synthio_note_set_waveform_loop_end_obj);


//|     envelope: Envelope
//|     """The envelope of this note"""
//|
static mp_obj_t synthio_note_get_envelope(mp_obj_t self_in) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return common_hal_synthio_note_get_envelope_obj(self);
}
MP_DEFINE_CONST_FUN_OBJ_1(synthio_note_get_envelope_obj, synthio_note_get_envelope);

static mp_obj_t synthio_note_set_envelope(mp_obj_t self_in, mp_obj_t arg) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_synthio_note_set_envelope(self, arg);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(synthio_note_set_envelope_obj, synthio_note_set_envelope);
MP_PROPERTY_GETSET(synthio_note_envelope_obj,
    (mp_obj_t)&synthio_note_get_envelope_obj,
    (mp_obj_t)&synthio_note_set_envelope_obj);

//|     ring_frequency: float
//|     """The ring frequency of the note, in Hz. Zero disables.
//|
//|     For ring to take effect, both ``ring_frequency`` and ``ring_waveform`` must be set."""
static mp_obj_t synthio_note_get_ring_frequency(mp_obj_t self_in) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_float(common_hal_synthio_note_get_ring_frequency(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(synthio_note_get_ring_frequency_obj, synthio_note_get_ring_frequency);

static mp_obj_t synthio_note_set_ring_frequency(mp_obj_t self_in, mp_obj_t arg) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_synthio_note_set_ring_frequency(self, mp_obj_get_float(arg));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(synthio_note_set_ring_frequency_obj, synthio_note_set_ring_frequency);
MP_PROPERTY_GETSET(synthio_note_ring_frequency_obj,
    (mp_obj_t)&synthio_note_get_ring_frequency_obj,
    (mp_obj_t)&synthio_note_set_ring_frequency_obj);

//|     ring_bend: float
//|     """The pitch bend depth of the note's ring waveform, from -12 to +12
//|
//|     A depth of 0 plays the programmed frequency. A depth of 1 corresponds to a bend of 1
//|     octave.  A depth of (1/12) = 0.0833 corresponds to a bend of 1 semitone,
//|     and a depth of .00833 corresponds to one musical cent.
//|
//|     To achieve a vibrato or sweep effect on the ring waveform, attach an LFO here.
//|     """
static mp_obj_t synthio_note_get_ring_bend(mp_obj_t self_in) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return common_hal_synthio_note_get_ring_bend(self);
}
MP_DEFINE_CONST_FUN_OBJ_1(synthio_note_get_ring_bend_obj, synthio_note_get_ring_bend);

static mp_obj_t synthio_note_set_ring_bend(mp_obj_t self_in, mp_obj_t arg) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_synthio_note_set_ring_bend(self, arg);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(synthio_note_set_ring_bend_obj, synthio_note_set_ring_bend);
MP_PROPERTY_GETSET(synthio_note_ring_bend_obj,
    (mp_obj_t)&synthio_note_get_ring_bend_obj,
    (mp_obj_t)&synthio_note_set_ring_bend_obj);

//|     ring_waveform: Optional[ReadableBuffer]
//|     """The ring waveform of this note. Setting the ring_waveform to a buffer of a different size resets the note's phase.
//|
//|     For ring to take effect, both ``ring_frequency`` and ``ring_waveform`` must be set."""
//|
static mp_obj_t synthio_note_get_ring_waveform(mp_obj_t self_in) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return common_hal_synthio_note_get_ring_waveform_obj(self);
}
MP_DEFINE_CONST_FUN_OBJ_1(synthio_note_get_ring_waveform_obj, synthio_note_get_ring_waveform);

static mp_obj_t synthio_note_set_ring_waveform(mp_obj_t self_in, mp_obj_t arg) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_synthio_note_set_ring_waveform(self, arg);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(synthio_note_set_ring_waveform_obj, synthio_note_set_ring_waveform);
MP_PROPERTY_GETSET(synthio_note_ring_waveform_obj,
    (mp_obj_t)&synthio_note_get_ring_waveform_obj,
    (mp_obj_t)&synthio_note_set_ring_waveform_obj);

//|     ring_waveform_loop_start: int
//|     """The sample index of where to begin looping waveform data.
//|
//|     Values outside the range ``0`` to ``waveform_max_length-1`` (inclusive) are rejected with a `ValueError`.
//|
//|     Values greater than or equal to the actual waveform length are treated as 0."""
static mp_obj_t synthio_note_get_ring_waveform_loop_start(mp_obj_t self_in) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(common_hal_synthio_note_get_ring_waveform_loop_start(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(synthio_note_get_ring_waveform_loop_start_obj, synthio_note_get_ring_waveform_loop_start);

static mp_obj_t synthio_note_set_ring_waveform_loop_start(mp_obj_t self_in, mp_obj_t arg) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_synthio_note_set_ring_waveform_loop_start(self, mp_obj_get_int(arg));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(synthio_note_set_ring_waveform_loop_start_obj, synthio_note_set_ring_waveform_loop_start);
MP_PROPERTY_GETSET(synthio_note_ring_waveform_loop_start_obj,
    (mp_obj_t)&synthio_note_get_ring_waveform_loop_start_obj,
    (mp_obj_t)&synthio_note_set_ring_waveform_loop_start_obj);

//|     ring_waveform_loop_end: int
//|     """The sample index of where to end looping waveform data.
//|
//|     Values outside the range ``1`` to ``waveform_max_length`` (inclusive) are rejected with a `ValueError`.
//|
//|     If the value is greater than the actual waveform length, or less than or equal to the loop start, the loop will occur at the end of the waveform.
//|
//|     Use the `synthio.waveform_max_length` constant to set the loop point at the end of the wave form, no matter its length."""
//|
static mp_obj_t synthio_note_get_ring_waveform_loop_end(mp_obj_t self_in) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(common_hal_synthio_note_get_ring_waveform_loop_end(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(synthio_note_get_ring_waveform_loop_end_obj, synthio_note_get_ring_waveform_loop_end);

static mp_obj_t synthio_note_set_ring_waveform_loop_end(mp_obj_t self_in, mp_obj_t arg) {
    synthio_note_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_synthio_note_set_ring_waveform_loop_end(self, mp_obj_get_int(arg));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(synthio_note_set_ring_waveform_loop_end_obj, synthio_note_set_ring_waveform_loop_end);
MP_PROPERTY_GETSET(synthio_note_ring_waveform_loop_end_obj,
    (mp_obj_t)&synthio_note_get_ring_waveform_loop_end_obj,
    (mp_obj_t)&synthio_note_set_ring_waveform_loop_end_obj);



static void note_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    properties_print_helper(print, self_in, note_properties, MP_ARRAY_SIZE(note_properties));
}

static const mp_rom_map_elem_t synthio_note_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_frequency), MP_ROM_PTR(&synthio_note_frequency_obj) },
    { MP_ROM_QSTR(MP_QSTR_filter), MP_ROM_PTR(&synthio_note_filter_obj) },
    { MP_ROM_QSTR(MP_QSTR_panning), MP_ROM_PTR(&synthio_note_panning_obj) },
    { MP_ROM_QSTR(MP_QSTR_waveform), MP_ROM_PTR(&synthio_note_waveform_obj) },
    { MP_ROM_QSTR(MP_QSTR_waveform_loop_start), MP_ROM_PTR(&synthio_note_waveform_loop_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_waveform_loop_end), MP_ROM_PTR(&synthio_note_waveform_loop_end_obj) },
    { MP_ROM_QSTR(MP_QSTR_envelope), MP_ROM_PTR(&synthio_note_envelope_obj) },
    { MP_ROM_QSTR(MP_QSTR_amplitude), MP_ROM_PTR(&synthio_note_amplitude_obj) },
    { MP_ROM_QSTR(MP_QSTR_bend), MP_ROM_PTR(&synthio_note_bend_obj) },
    { MP_ROM_QSTR(MP_QSTR_ring_frequency), MP_ROM_PTR(&synthio_note_ring_frequency_obj) },
    { MP_ROM_QSTR(MP_QSTR_ring_bend), MP_ROM_PTR(&synthio_note_ring_bend_obj) },
    { MP_ROM_QSTR(MP_QSTR_ring_waveform), MP_ROM_PTR(&synthio_note_ring_waveform_obj) },
    { MP_ROM_QSTR(MP_QSTR_ring_waveform_loop_start), MP_ROM_PTR(&synthio_note_ring_waveform_loop_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_ring_waveform_loop_end), MP_ROM_PTR(&synthio_note_ring_waveform_loop_end_obj) },
};
static MP_DEFINE_CONST_DICT(synthio_note_locals_dict, synthio_note_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    synthio_note_type,
    MP_QSTR_Note,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    make_new, synthio_note_make_new,
    locals_dict, &synthio_note_locals_dict,
    print, note_print
    );
