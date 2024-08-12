// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Lucian Copeland for Adafruit Industries
//
// SPDX-License-Identifier: MIT
#include <math.h>

#include "common-hal/pwmio/PWMOut.h"
#include "shared-bindings/pwmio/PWMOut.h"
#include "py/runtime.h"
#include "driver/ledc.h"
#include "soc/soc.h"

#define INDEX_EMPTY 0xFF

static uint32_t reserved_timer_freq[LEDC_TIMER_MAX];
static bool varfreq_timers[LEDC_TIMER_MAX];
static uint8_t reserved_channels[LEDC_CHANNEL_MAX] = { [0 ... LEDC_CHANNEL_MAX - 1] = INDEX_EMPTY};

static uint32_t calculate_duty_cycle(uint32_t frequency) {
    uint32_t duty_bits = 0;
    uint32_t interval = APB_CLK_FREQ / frequency;
    for (size_t i = 0; i < 32; i++) {
        if (!(interval >> i)) {
            duty_bits = i - 1;
            break;
        }
    }
    if (duty_bits >= LEDC_TIMER_14_BIT) {
        duty_bits = LEDC_TIMER_13_BIT;
    }
    return duty_bits;
}

pwmout_result_t common_hal_pwmio_pwmout_construct(pwmio_pwmout_obj_t *self,
    const mcu_pin_obj_t *pin,
    uint16_t duty,
    uint32_t frequency,
    bool variable_frequency) {

    // check the frequency (avoid divide by zero below)
    if (frequency == 0) {
        return PWMOUT_INVALID_FREQUENCY;
    }

    // Calculate duty cycle
    uint32_t duty_bits = calculate_duty_cycle(frequency);
    if (duty_bits == 0) {
        return PWMOUT_INVALID_FREQUENCY;
    }

    // Find a viable timer
    size_t timer_index = INDEX_EMPTY;
    size_t channel_index = INDEX_EMPTY;
    for (size_t i = 0; i < LEDC_TIMER_MAX; i++) {
        // accept matching freq timers unless this instance is varfreq or a prior one was
        if ((reserved_timer_freq[i] == frequency) && !variable_frequency && !varfreq_timers[i]) {
            // prioritize matched frequencies so we don't needlessly take slots
            timer_index = i;
            break;
        } else if (reserved_timer_freq[i] == 0) {
            timer_index = i;
            break;
        }
    }
    if (timer_index == INDEX_EMPTY) {
        // Running out of timers isn't pin related on ESP32S2.
        return PWMOUT_INTERNAL_RESOURCES_IN_USE;
    }

    // Find a viable channel
    for (size_t i = 0; i < LEDC_CHANNEL_MAX; i++) {
        if (reserved_channels[i] == INDEX_EMPTY) {
            channel_index = i;
            break;
        }
    }
    if (channel_index == INDEX_EMPTY) {
        return PWMOUT_INTERNAL_RESOURCES_IN_USE;
    }

    // Run configuration
    self->tim_handle.timer_num = timer_index;
    self->tim_handle.duty_resolution = duty_bits;
    self->tim_handle.freq_hz = frequency;
    self->tim_handle.speed_mode = LEDC_LOW_SPEED_MODE;
    self->tim_handle.clk_cfg = LEDC_AUTO_CLK;

    if (ledc_timer_config(&(self->tim_handle)) != ESP_OK) {
        return PWMOUT_INITIALIZATION_ERROR;
    }

    self->chan_handle.channel = channel_index;
    self->chan_handle.duty = duty >> (16 - duty_bits);
    self->chan_handle.gpio_num = pin->number;
    self->chan_handle.speed_mode = LEDC_LOW_SPEED_MODE; // Only LS is allowed on ESP32-S2
    self->chan_handle.hpoint = 0;
    self->chan_handle.timer_sel = timer_index;

    if (ledc_channel_config(&(self->chan_handle))) {
        return PWMOUT_INITIALIZATION_ERROR;
    }

    // Make reservations
    reserved_timer_freq[timer_index] = frequency;
    reserved_channels[channel_index] = timer_index;

    if (variable_frequency) {
        varfreq_timers[timer_index] = true;
    }
    self->variable_frequency = variable_frequency;
    self->pin = pin;
    self->deinited = false;
    self->duty_resolution = duty_bits;
    claim_pin(pin);

    // Set initial duty
    common_hal_pwmio_pwmout_set_duty_cycle(self, duty);

    return PWMOUT_OK;
}

void common_hal_pwmio_pwmout_never_reset(pwmio_pwmout_obj_t *self) {
    never_reset_pin_number(self->pin->number);
}

bool common_hal_pwmio_pwmout_deinited(pwmio_pwmout_obj_t *self) {
    return self->deinited == true;
}

void common_hal_pwmio_pwmout_deinit(pwmio_pwmout_obj_t *self) {
    if (common_hal_pwmio_pwmout_deinited(self)) {
        return;
    }

    if (reserved_channels[self->chan_handle.channel] != INDEX_EMPTY) {
        ledc_stop(LEDC_LOW_SPEED_MODE, self->chan_handle.channel, 0);
    }
    reserved_channels[self->chan_handle.channel] = INDEX_EMPTY;

    // Search if any other channel is using the timer
    bool taken = false;
    for (size_t i = 0; i < LEDC_CHANNEL_MAX; i++) {
        if (reserved_channels[i] == self->tim_handle.timer_num) {
            taken = true;
            break;
        }
    }
    // Variable frequency means there's only one channel on the timer
    if (!taken || self->variable_frequency) {
        ledc_timer_rst(LEDC_LOW_SPEED_MODE, self->tim_handle.timer_num);
        reserved_timer_freq[self->tim_handle.timer_num] = 0;
        // if timer isn't varfreq this will be off already
        varfreq_timers[self->tim_handle.timer_num] = false;
    }
    common_hal_reset_pin(self->pin);
    self->deinited = true;
}

void common_hal_pwmio_pwmout_set_duty_cycle(pwmio_pwmout_obj_t *self, uint16_t duty) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, self->chan_handle.channel, duty >> (16 - self->duty_resolution));
    ledc_update_duty(LEDC_LOW_SPEED_MODE, self->chan_handle.channel);
}

uint16_t common_hal_pwmio_pwmout_get_duty_cycle(pwmio_pwmout_obj_t *self) {
    return ledc_get_duty(LEDC_LOW_SPEED_MODE, self->chan_handle.channel) << (16 - self->duty_resolution);
}

void common_hal_pwmio_pwmout_set_frequency(pwmio_pwmout_obj_t *self, uint32_t frequency) {
    // Calculate duty cycle
    uint32_t duty_bits = calculate_duty_cycle(frequency);
    if (duty_bits == 0) {
        mp_arg_error_invalid(MP_QSTR_frequency);
    }
    self->duty_resolution = duty_bits;
    ledc_set_freq(LEDC_LOW_SPEED_MODE, self->tim_handle.timer_num, frequency);
}

uint32_t common_hal_pwmio_pwmout_get_frequency(pwmio_pwmout_obj_t *self) {
    return ledc_get_freq(LEDC_LOW_SPEED_MODE, self->tim_handle.timer_num);
}

bool common_hal_pwmio_pwmout_get_variable_frequency(pwmio_pwmout_obj_t *self) {
    return self->variable_frequency;
}

const mcu_pin_obj_t *common_hal_pwmio_pwmout_get_pin(pwmio_pwmout_obj_t *self) {
    return self->pin;
}
