// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2023 Jeff Epler for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#include "shared-module/synthio/__init__.h"
#include "shared-module/synthio/Biquad.h"
#include "shared-module/synthio/LFO.h"
#include "shared-bindings/synthio/__init__.h"

typedef struct synthio_note_obj {
    mp_obj_base_t base;

    synthio_block_slot_t panning, bend, amplitude, ring_bend;

    mp_float_t frequency, ring_frequency;
    mp_obj_t waveform_obj, envelope_obj, ring_waveform_obj;
    mp_obj_t filter_obj;

    biquad_filter_state filter_state;

    int32_t sample_rate;

    int32_t frequency_scaled;
    int32_t ring_frequency_scaled, ring_frequency_bent;

    mp_buffer_info_t waveform_buf;
    uint32_t waveform_loop_start, waveform_loop_end;
    mp_buffer_info_t ring_waveform_buf;
    uint32_t ring_waveform_loop_start, ring_waveform_loop_end;
    synthio_envelope_definition_t envelope_def;
} synthio_note_obj_t;

void synthio_note_recalculate(synthio_note_obj_t *self, int32_t sample_rate);
uint32_t synthio_note_step(synthio_note_obj_t *self, int32_t sample_rate, int16_t dur, int16_t loudness[2]);
void synthio_note_start(synthio_note_obj_t *self, int32_t sample_rate);
bool synthio_note_playing(synthio_note_obj_t *self);
