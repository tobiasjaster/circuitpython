// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017-2020 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/digitalio/DigitalInOut.h"
#include "py/runtime.h"

#include "components/driver/gpio/include/driver/gpio.h"

#include "components/hal/include/hal/gpio_hal.h"

static bool _pin_is_input(uint8_t pin_number) {
    const uint32_t iomux = READ_PERI_REG(GPIO_PIN_MUX_REG[pin_number]);
    return (iomux & FUN_IE) != 0;
}

void digitalio_digitalinout_preserve_for_deep_sleep(size_t n_dios, digitalio_digitalinout_obj_t *preserve_dios[]) {
    // Mark the pin states of the given DigitalInOuts for preservation during deep sleep
    for (size_t i = 0; i < n_dios; i++) {
        if (!common_hal_digitalio_digitalinout_deinited(preserve_dios[i])) {
            preserve_pin_number(preserve_dios[i]->pin->number);
        }
    }
}

void common_hal_digitalio_digitalinout_never_reset(
    digitalio_digitalinout_obj_t *self) {
    never_reset_pin_number(self->pin->number);
}

digitalinout_result_t common_hal_digitalio_digitalinout_construct(
    digitalio_digitalinout_obj_t *self, const mcu_pin_obj_t *pin) {
    claim_pin(pin);
    self->pin = pin;

    gpio_config_t config;
    config.pin_bit_mask = 1ull << pin->number;
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    if (gpio_config(&config) != ESP_OK) {
        return DIGITALINOUT_PIN_BUSY;
    }

    return DIGITALINOUT_OK;
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
    common_hal_digitalio_digitalinout_set_pull(self, pull);
    gpio_set_direction(self->pin->number, GPIO_MODE_INPUT);
    return DIGITALINOUT_OK;
}

digitalinout_result_t common_hal_digitalio_digitalinout_switch_to_output(
    digitalio_digitalinout_obj_t *self, bool value,
    digitalio_drive_mode_t drive_mode) {
    common_hal_digitalio_digitalinout_set_value(self, value);
    return common_hal_digitalio_digitalinout_set_drive_mode(self, drive_mode);
}

digitalio_direction_t common_hal_digitalio_digitalinout_get_direction(
    digitalio_digitalinout_obj_t *self) {
    if (_pin_is_input(self->pin->number)) {
        return DIRECTION_INPUT;
    }
    return DIRECTION_OUTPUT;
}

void common_hal_digitalio_digitalinout_set_value(
    digitalio_digitalinout_obj_t *self, bool value) {
    self->output_value = value;
    gpio_set_level(self->pin->number, value);
}

bool common_hal_digitalio_digitalinout_get_value(
    digitalio_digitalinout_obj_t *self) {
    if (common_hal_digitalio_digitalinout_get_direction(self) == DIRECTION_INPUT) {
        return gpio_get_level(self->pin->number) == 1;
    }
    return self->output_value;
}

digitalinout_result_t common_hal_digitalio_digitalinout_set_drive_mode(
    digitalio_digitalinout_obj_t *self,
    digitalio_drive_mode_t drive_mode) {
    gpio_num_t number = self->pin->number;
    gpio_mode_t mode = GPIO_MODE_OUTPUT;
    if (drive_mode == DRIVE_MODE_OPEN_DRAIN) {
        mode |= GPIO_MODE_OUTPUT_OD;
    }
    esp_err_t result = gpio_set_direction(number, mode);
    if (result != ESP_OK) {
        return DIGITALINOUT_INPUT_ONLY;
    }
    return DIGITALINOUT_OK;
}

digitalio_drive_mode_t common_hal_digitalio_digitalinout_get_drive_mode(
    digitalio_digitalinout_obj_t *self) {
    if (GPIO_HAL_GET_HW(GPIO_PORT_0)->pin[self->pin->number].pad_driver == 1) {
        return DRIVE_MODE_OPEN_DRAIN;
    }
    return DRIVE_MODE_PUSH_PULL;
}

digitalinout_result_t common_hal_digitalio_digitalinout_set_pull(
    digitalio_digitalinout_obj_t *self, digitalio_pull_t pull) {
    gpio_num_t number = self->pin->number;
    gpio_pullup_dis(number);
    gpio_pulldown_dis(number);
    if (pull == PULL_UP) {
        gpio_pullup_en(number);
    } else if (pull == PULL_DOWN) {
        gpio_pulldown_en(number);
    }
    return DIGITALINOUT_OK;
}

digitalio_pull_t common_hal_digitalio_digitalinout_get_pull(
    digitalio_digitalinout_obj_t *self) {
    gpio_num_t gpio_num = self->pin->number;
    if (REG_GET_BIT(GPIO_PIN_MUX_REG[gpio_num], FUN_PU)) {
        return PULL_UP;
    } else if (REG_GET_BIT(GPIO_PIN_MUX_REG[gpio_num], FUN_PD)) {
        return PULL_DOWN;
    }
    return PULL_NONE;
}
