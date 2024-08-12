// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2018 DeanM for Adafruit Industries
//
// SPDX-License-Identifier: MIT
#include "shared-bindings/audiomixer/Mixer.h"
#include "shared-bindings/audiomixer/MixerVoice.h"
#include "shared-module/audiomixer/MixerVoice.h"

#include <stdint.h>

#include "py/runtime.h"
#include "shared-module/audiomixer/__init__.h"
#include "shared-module/audiocore/RawSample.h"

void common_hal_audiomixer_mixervoice_construct(audiomixer_mixervoice_obj_t *self) {
    self->sample = NULL;
    self->level = 1 << 15;
}

void common_hal_audiomixer_mixervoice_set_parent(audiomixer_mixervoice_obj_t *self, audiomixer_mixer_obj_t *parent) {
    self->parent = parent;
}

mp_float_t common_hal_audiomixer_mixervoice_get_level(audiomixer_mixervoice_obj_t *self) {
    return (mp_float_t)self->level / (1 << 15);
}

void common_hal_audiomixer_mixervoice_set_level(audiomixer_mixervoice_obj_t *self, mp_float_t level) {
    self->level = (uint16_t)(level * (1 << 15));
}

void common_hal_audiomixer_mixervoice_play(audiomixer_mixervoice_obj_t *self, mp_obj_t sample, bool loop) {
    if (audiosample_sample_rate(sample) != self->parent->sample_rate) {
        mp_raise_ValueError(MP_ERROR_TEXT("The sample's sample rate does not match the mixer's"));
    }
    if (audiosample_channel_count(sample) != self->parent->channel_count) {
        mp_raise_ValueError(MP_ERROR_TEXT("The sample's channel count does not match the mixer's"));
    }
    if (audiosample_bits_per_sample(sample) != self->parent->bits_per_sample) {
        mp_raise_ValueError(MP_ERROR_TEXT("The sample's bits_per_sample does not match the mixer's"));
    }
    bool single_buffer;
    bool samples_signed;
    uint32_t max_buffer_length;
    uint8_t spacing;
    audiosample_get_buffer_structure(sample, false, &single_buffer, &samples_signed,
        &max_buffer_length, &spacing);
    if (samples_signed != self->parent->samples_signed) {
        mp_raise_ValueError(MP_ERROR_TEXT("The sample's signedness does not match the mixer's"));
    }
    self->sample = sample;
    self->loop = loop;

    audiosample_reset_buffer(sample, false, 0);
    audioio_get_buffer_result_t result = audiosample_get_buffer(sample, false, 0, (uint8_t **)&self->remaining_buffer, &self->buffer_length);
    // Track length in terms of words.
    self->buffer_length /= sizeof(uint32_t);
    self->more_data = result == GET_BUFFER_MORE_DATA;
}

bool common_hal_audiomixer_mixervoice_get_playing(audiomixer_mixervoice_obj_t *self) {
    return self->sample != NULL;
}

void common_hal_audiomixer_mixervoice_stop(audiomixer_mixervoice_obj_t *self) {
    self->sample = NULL;
}
