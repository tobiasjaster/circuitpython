// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright 2019 Sony Semiconductor Solutions Corporation
//
// SPDX-License-Identifier: MIT

#include <arch/board/board.h>
#include <sys/time.h>

#include "py/runtime.h"
#include "py/mphal.h"

#include "shared-bindings/pulseio/PulseIn.h"
#include "shared-bindings/microcontroller/__init__.h"

static pulseio_pulsein_obj_t *pulsein_objects[12];

static int pulsein_interrupt_handler(int irq, FAR void *context, FAR void *arg);

static int pulsein_set_config(pulseio_pulsein_obj_t *self, bool first_edge) {
    int mode;

    if (!first_edge) {
        mode = INT_BOTH_EDGE;
    } else if (self->idle_state) {
        mode = INT_FALLING_EDGE;
    } else {
        mode = INT_RISING_EDGE;
    }
    return board_gpio_intconfig(self->pin->number, mode, false, pulsein_interrupt_handler);
}

static int pulsein_interrupt_handler(int irq, FAR void *context, FAR void *arg) {
    // Grab the current time first.
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t current_us = ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;

    pulseio_pulsein_obj_t *self = pulsein_objects[irq - CXD56_IRQ_EXDEVICE_0];

    if (self->first_edge) {
        self->first_edge = false;
        pulsein_set_config(self, false);
        board_gpio_int(self->pin->number, true);
    } else {
        uint32_t us_diff = current_us - self->last_us;

        uint16_t duration = 0xffff;
        if (us_diff < duration) {
            duration = us_diff;
        }

        uint16_t i = (self->start + self->len) % self->maxlen;
        self->buffer[i] = duration;
        if (self->len < self->maxlen) {
            self->len++;
        } else {
            self->start = (self->start + 1) % self->maxlen;
        }
    }
    self->last_us = current_us;

    return 0;
}

void common_hal_pulseio_pulsein_construct(pulseio_pulsein_obj_t *self,
    const mcu_pin_obj_t *pin, uint16_t maxlen, bool idle_state) {
    self->buffer = (uint16_t *)m_malloc(maxlen * sizeof(uint16_t));
    if (self->buffer == NULL) {
        m_malloc_fail(maxlen * sizeof(uint16_t));
    }

    self->pin = pin;
    self->maxlen = maxlen;
    self->idle_state = idle_state;
    self->start = 0;
    self->len = 0;
    self->first_edge = true;
    self->paused = false;

    int irq = pulsein_set_config(self, true);
    if (irq < 0) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Internal resource(s) in use"));
    } else {
        pulsein_objects[irq - CXD56_IRQ_EXDEVICE_0] = self;
    }

    claim_pin(pin);

    board_gpio_int(self->pin->number, true);
}

void common_hal_pulseio_pulsein_deinit(pulseio_pulsein_obj_t *self) {
    if (common_hal_pulseio_pulsein_deinited(self)) {
        return;
    }

    board_gpio_int(self->pin->number, false);
    board_gpio_intconfig(self->pin->number, 0, false, NULL);

    reset_pin_number(self->pin->number);
    self->pin = NULL;
}

bool common_hal_pulseio_pulsein_deinited(pulseio_pulsein_obj_t *self) {
    return self->pin == NULL;
}

void common_hal_pulseio_pulsein_pause(pulseio_pulsein_obj_t *self) {
    board_gpio_int(self->pin->number, false);
    self->paused = true;
}

void common_hal_pulseio_pulsein_resume(pulseio_pulsein_obj_t *self, uint16_t trigger_duration) {
    // Make sure we're paused.
    common_hal_pulseio_pulsein_pause(self);

    // Send the trigger pulse.
    if (trigger_duration > 0) {
        board_gpio_config(self->pin->number, 0, false, true, PIN_FLOAT);
        board_gpio_write(self->pin->number, !self->idle_state);
        common_hal_mcu_delay_us((uint32_t)trigger_duration);
        board_gpio_write(self->pin->number, self->idle_state);
    }

    // Reconfigure the pin and make sure its set to detect the first edge.
    self->first_edge = true;
    self->paused = false;

    pulsein_set_config(self, true);
    board_gpio_int(self->pin->number, true);
}

void common_hal_pulseio_pulsein_clear(pulseio_pulsein_obj_t *self) {
    common_hal_mcu_disable_interrupts();
    self->start = 0;
    self->len = 0;
    common_hal_mcu_enable_interrupts();
}

uint16_t common_hal_pulseio_pulsein_popleft(pulseio_pulsein_obj_t *self) {
    if (self->len == 0) {
        mp_raise_IndexError_varg(MP_ERROR_TEXT("pop from empty %q"), MP_QSTR_PulseIn);
    }
    common_hal_mcu_disable_interrupts();
    uint16_t value = self->buffer[self->start];
    self->start = (self->start + 1) % self->maxlen;
    self->len--;
    common_hal_mcu_enable_interrupts();

    return value;
}

uint16_t common_hal_pulseio_pulsein_get_maxlen(pulseio_pulsein_obj_t *self) {
    return self->maxlen;
}

bool common_hal_pulseio_pulsein_get_paused(pulseio_pulsein_obj_t *self) {
    return self->paused;
}

uint16_t common_hal_pulseio_pulsein_get_len(pulseio_pulsein_obj_t *self) {
    return self->len;
}

uint16_t common_hal_pulseio_pulsein_get_item(pulseio_pulsein_obj_t *self, int16_t index) {
    common_hal_mcu_disable_interrupts();
    if (index < 0) {
        index += self->len;
    }
    if (index < 0 || index >= self->len) {
        common_hal_mcu_enable_interrupts();
        mp_raise_IndexError_varg(MP_ERROR_TEXT("%q out of range"), MP_QSTR_index);
    }
    uint16_t value = self->buffer[(self->start + index) % self->maxlen];
    common_hal_mcu_enable_interrupts();
    return value;
}
