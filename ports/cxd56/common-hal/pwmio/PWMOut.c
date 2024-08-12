// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright 2019 Sony Semiconductor Solutions Corporation
//
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "py/runtime.h"

#include "shared-bindings/pwmio/PWMOut.h"

typedef struct {
    const char *devpath;
    const mcu_pin_obj_t *pin;
    int fd;
    bool reset;
} pwmout_dev_t;

static pwmout_dev_t pwmout_dev[] = {
    {"/dev/pwm0", &pin_PWM0, -1, true},
    {"/dev/pwm1", &pin_PWM1, -1, true},
    {"/dev/pwm2", &pin_PWM2, -1, true},
    {"/dev/pwm3", &pin_PWM3, -1, true}
};

pwmout_result_t common_hal_pwmio_pwmout_construct(pwmio_pwmout_obj_t *self,
    const mcu_pin_obj_t *pin, uint16_t duty, uint32_t frequency,
    bool variable_frequency) {
    self->number = -1;

    for (int i = 0; i < MP_ARRAY_SIZE(pwmout_dev); i++) {
        if (pin->number == pwmout_dev[i].pin->number) {
            self->number = i;
            break;
        }
    }

    if (self->number < 0) {
        return PWMOUT_INVALID_PIN;
    }

    if (pwmout_dev[self->number].fd < 0) {
        pwmout_dev[self->number].fd = open(pwmout_dev[self->number].devpath, O_RDONLY);
        if (pwmout_dev[self->number].fd < 0) {
            return PWMOUT_INVALID_PIN;
        }
    }

    self->info.frequency = frequency;
    self->info.duty = duty;
    self->variable_frequency = variable_frequency;

    if (ioctl(pwmout_dev[self->number].fd, PWMIOC_SETCHARACTERISTICS, (unsigned long)((uintptr_t)&self->info)) < 0) {
        mp_arg_error_invalid(MP_QSTR_frequency);
    }
    ioctl(pwmout_dev[self->number].fd, PWMIOC_START, 0);

    claim_pin(pin);

    self->pin = pin;

    return PWMOUT_OK;
}

void common_hal_pwmio_pwmout_deinit(pwmio_pwmout_obj_t *self) {
    if (common_hal_pwmio_pwmout_deinited(self)) {
        return;
    }

    pwmout_dev[self->number].reset = true;

    ioctl(pwmout_dev[self->number].fd, PWMIOC_STOP, 0);
    close(pwmout_dev[self->number].fd);
    pwmout_dev[self->number].fd = -1;

    reset_pin_number(self->pin->number);
    self->pin = NULL;
}

bool common_hal_pwmio_pwmout_deinited(pwmio_pwmout_obj_t *self) {
    return pwmout_dev[self->number].fd < 0;
}

void common_hal_pwmio_pwmout_set_duty_cycle(pwmio_pwmout_obj_t *self, uint16_t duty) {
    self->info.duty = duty;

    ioctl(pwmout_dev[self->number].fd, PWMIOC_SETCHARACTERISTICS, (unsigned long)((uintptr_t)&self->info));
}

uint16_t common_hal_pwmio_pwmout_get_duty_cycle(pwmio_pwmout_obj_t *self) {
    return self->info.duty;
}

void common_hal_pwmio_pwmout_set_frequency(pwmio_pwmout_obj_t *self, uint32_t frequency) {
    self->info.frequency = frequency;

    if (ioctl(pwmout_dev[self->number].fd, PWMIOC_SETCHARACTERISTICS, (unsigned long)((uintptr_t)&self->info)) < 0) {
        mp_arg_error_invalid(MP_QSTR_frequency);
    }
}

uint32_t common_hal_pwmio_pwmout_get_frequency(pwmio_pwmout_obj_t *self) {
    return self->info.frequency;
}

bool common_hal_pwmio_pwmout_get_variable_frequency(pwmio_pwmout_obj_t *self) {
    return self->variable_frequency;
}

void common_hal_pwmio_pwmout_never_reset(pwmio_pwmout_obj_t *self) {
    never_reset_pin_number(self->pin->number);

    pwmout_dev[self->number].reset = false;
}

void pwmout_start(uint8_t pwm_num) {
    ioctl(pwmout_dev[pwm_num].fd, PWMIOC_START, 0);
}

void pwmout_stop(uint8_t pwm_num) {
    ioctl(pwmout_dev[pwm_num].fd, PWMIOC_STOP, 0);
}

const mcu_pin_obj_t *common_hal_pwmio_pwmout_get_pin(pwmio_pwmout_obj_t *self) {
    return self->pin;
}
