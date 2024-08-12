// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <stdint.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"

#include "common-hal/microcontroller/Pin.h"
#include "shared-bindings/digitalio/DigitalInOut.h"

#include "peripherals/broadcom/gpio.h"

digitalinout_result_t common_hal_digitalio_digitalinout_construct(
    digitalio_digitalinout_obj_t *self, const mcu_pin_obj_t *pin) {
    claim_pin(pin);
    self->pin = pin;
    self->output = false;
    self->open_drain = false;

    return DIGITALINOUT_OK;
}

void common_hal_digitalio_digitalinout_never_reset(
    digitalio_digitalinout_obj_t *self) {
    never_reset_pin_number(self->pin->number);
}

bool common_hal_digitalio_digitalinout_deinited(digitalio_digitalinout_obj_t *self) {
    return self->pin == NULL;
}

void common_hal_digitalio_digitalinout_deinit(digitalio_digitalinout_obj_t *self) {
    if (common_hal_digitalio_digitalinout_deinited(self)) {
        return;
    }
    reset_pin_number(self->pin->number);
    self->pin = NULL;
}

digitalinout_result_t common_hal_digitalio_digitalinout_switch_to_input(
    digitalio_digitalinout_obj_t *self, digitalio_pull_t pull) {
    self->output = false;
    // This also sets direction to input.
    common_hal_digitalio_digitalinout_set_pull(self, pull);
    return DIGITALINOUT_OK;
}

digitalinout_result_t common_hal_digitalio_digitalinout_switch_to_output(
    digitalio_digitalinout_obj_t *self, bool value,
    digitalio_drive_mode_t drive_mode) {
    const uint8_t pin = self->pin->number;
    gpio_set_pull(pin, PULL_NONE);

    self->output = true;
    self->open_drain = drive_mode == DRIVE_MODE_OPEN_DRAIN;

    // Pin direction is ultimately set in set_value. We don't need to do it here.
    common_hal_digitalio_digitalinout_set_value(self, value);
    return DIGITALINOUT_OK;
}

digitalio_direction_t common_hal_digitalio_digitalinout_get_direction(
    digitalio_digitalinout_obj_t *self) {
    return self->output ? DIRECTION_OUTPUT : DIRECTION_INPUT;
}

void common_hal_digitalio_digitalinout_set_value(
    digitalio_digitalinout_obj_t *self, bool value) {
    const uint8_t pin = self->pin->number;
    if (self->open_drain && value) {
        // If true and open-drain, set the direction -before- setting
        // the pin value, to to avoid a high glitch on the pin before
        // switching from output to input for open-drain.
        gpio_set_function(pin, GPIO_FUNCTION_INPUT);
        gpio_set_value(pin, value);
    } else {
        // Otherwise set the direction -after- setting the pin value,
        // to avoid a glitch which might occur if the old value was
        // different and the pin was previously set to input.
        gpio_set_value(pin, value);
        gpio_set_function(pin, GPIO_FUNCTION_OUTPUT);
    }
}

bool common_hal_digitalio_digitalinout_get_value(
    digitalio_digitalinout_obj_t *self) {
    return gpio_get_value(self->pin->number);
}

digitalinout_result_t common_hal_digitalio_digitalinout_set_drive_mode(
    digitalio_digitalinout_obj_t *self,
    digitalio_drive_mode_t drive_mode) {
    bool value = common_hal_digitalio_digitalinout_get_value(self);
    self->open_drain = drive_mode == DRIVE_MODE_OPEN_DRAIN;
    // True is implemented differently between modes so reset the value to make
    // sure it's correct for the new mode.
    if (value) {
        common_hal_digitalio_digitalinout_set_value(self, value);
    }
    return DIGITALINOUT_OK;
}

digitalio_drive_mode_t common_hal_digitalio_digitalinout_get_drive_mode(
    digitalio_digitalinout_obj_t *self) {
    if (self->open_drain) {
        return DRIVE_MODE_OPEN_DRAIN;
    } else {
        return DRIVE_MODE_PUSH_PULL;
    }
}

digitalinout_result_t common_hal_digitalio_digitalinout_set_pull(
    digitalio_digitalinout_obj_t *self, digitalio_pull_t pull) {
    const uint8_t pin = self->pin->number;
    BP_PULL_Enum bp_pull = BP_PULL_NONE;
    if (pull == PULL_UP) {
        bp_pull = BP_PULL_UP;
    } else if (pull == PULL_DOWN) {
        bp_pull = BP_PULL_DOWN;
    }
    gpio_set_pull(pin, bp_pull);
    return DIGITALINOUT_OK;
}

digitalio_pull_t common_hal_digitalio_digitalinout_get_pull(
    digitalio_digitalinout_obj_t *self) {
    uint32_t pin = self->pin->number;
    if (self->output) {
        mp_raise_AttributeError(MP_ERROR_TEXT("Cannot get pull while in output mode"));
        return PULL_NONE;
    } else {
        if (gpio_get_pull(pin) == BP_PULL_UP) {
            return PULL_UP;
        } else if (gpio_get_pull(pin) == BP_PULL_DOWN) {
            return PULL_DOWN;
        }
    }
    return PULL_NONE;
}
