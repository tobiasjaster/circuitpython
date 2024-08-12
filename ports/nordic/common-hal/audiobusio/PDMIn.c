// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Jeff Epler for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "common-hal/audiobusio/PDMIn.h"
#include "shared-bindings/audiobusio/PDMIn.h"
#include "shared-bindings/microcontroller/Pin.h"

#include "py/runtime.h"

__attribute__((used))
NRF_PDM_Type *nrf_pdm = NRF_PDM;

static uint32_t dummy_buffer[4];

// Caller validates that pins are free.
void common_hal_audiobusio_pdmin_construct(audiobusio_pdmin_obj_t *self,
    const mcu_pin_obj_t *clock_pin,
    const mcu_pin_obj_t *data_pin,
    uint32_t sample_rate,
    uint8_t bit_depth,
    bool mono,
    uint8_t oversample) {
    claim_pin(clock_pin);
    claim_pin(data_pin);

    self->mono = mono;
    self->clock_pin_number = clock_pin->number;
    self->data_pin_number = data_pin->number;

    if (sample_rate != 16000) {
        mp_raise_ValueError(MP_ERROR_TEXT("only sample_rate=16000 is supported"));
    }
    if (bit_depth != 16) {
        mp_raise_ValueError(MP_ERROR_TEXT("only bit_depth=16 is supported"));
    }
    nrf_pdm->PSEL.CLK = self->clock_pin_number;
    nrf_pdm->PSEL.DIN = self->data_pin_number;
    nrf_pdm->PDMCLKCTRL = PDM_PDMCLKCTRL_FREQ_Default; // For Ratio64
    nrf_pdm->RATIO = PDM_RATIO_RATIO_Ratio64;
    nrf_pdm->GAINL = PDM_GAINL_GAINL_DefaultGain;
    nrf_pdm->GAINR = PDM_GAINR_GAINR_DefaultGain;
    nrf_pdm->ENABLE = 1;

    nrf_pdm->SAMPLE.PTR = (uintptr_t)&dummy_buffer;
    nrf_pdm->SAMPLE.MAXCNT = 1;
    nrf_pdm->TASKS_START = 1;
}

bool common_hal_audiobusio_pdmin_deinited(audiobusio_pdmin_obj_t *self) {
    return self->clock_pin_number == NO_PIN;
}

void common_hal_audiobusio_pdmin_deinit(audiobusio_pdmin_obj_t *self) {
    nrf_pdm->ENABLE = 0;

    reset_pin_number(self->clock_pin_number);
    self->clock_pin_number = NO_PIN;
    reset_pin_number(self->data_pin_number);
    self->data_pin_number = NO_PIN;
}

uint8_t common_hal_audiobusio_pdmin_get_bit_depth(audiobusio_pdmin_obj_t *self) {
    return 16;
}

uint32_t common_hal_audiobusio_pdmin_get_sample_rate(audiobusio_pdmin_obj_t *self) {
    return 16000;
}

uint32_t common_hal_audiobusio_pdmin_record_to_buffer(audiobusio_pdmin_obj_t *self,
    uint16_t *output_buffer, uint32_t output_buffer_length) {
    // Note: Adafruit's module has SELECT pulled to GND, which makes the DATA
    // valid when the CLK is low, therefore it must be sampled on the rising edge.
    if (self->mono) {
        nrf_pdm->MODE = PDM_MODE_OPERATION_Stereo | PDM_MODE_EDGE_LeftRising;
    } else {
        nrf_pdm->MODE = PDM_MODE_OPERATION_Mono | PDM_MODE_EDGE_LeftRising;
    }

    // step 1. Redirect to real buffer
    nrf_pdm->SAMPLE.PTR = (uintptr_t)output_buffer;
    nrf_pdm->SAMPLE.MAXCNT = output_buffer_length;

    // a delay is the safest simple way to ensure that the above requested sample has started
    mp_hal_delay_us(200);
    nrf_pdm->EVENTS_END = 0;

    // step 2. Registers are double buffered, so pre-redirect back to dummy buffer
    nrf_pdm->SAMPLE.PTR = (uintptr_t)&dummy_buffer;
    nrf_pdm->SAMPLE.MAXCNT = 1;

    // Step 3. wait for PDM to end
    while (!nrf_pdm->EVENTS_END) {
        MICROPY_VM_HOOK_LOOP;
    }

    // Step 4. They want unsigned
    for (uint32_t i = 0; i < output_buffer_length; i++) {
        output_buffer[i] += 32768;
    }

    if (self->mono) {
        return (output_buffer_length / 2) * 2;
    } else {
        return (output_buffer_length / 4) * 4;
    }
}
