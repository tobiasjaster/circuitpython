// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Radomir Dopieralski
//
// SPDX-License-Identifier: MIT

#include <stdbool.h>

#include "py/mpstate.h"
#include "py/runtime.h"
#include "__init__.h"
#include "PewPew.h"

#include "shared-bindings/digitalio/Pull.h"
#include "shared-bindings/digitalio/DigitalInOut.h"
#include "shared-bindings/util.h"
#include "samd/timers.h"
#include "timer_handler.h"


static uint8_t pewpew_tc_index = 0xff;
static volatile uint16_t pewpew_ticks = 0;


void pewpew_interrupt_handler(uint8_t index) {
    if (index != pewpew_tc_index) {
        return;
    }
    Tc *tc = tc_insts[index];
    if (!tc->COUNT16.INTFLAG.bit.MC0) {
        return;
    }

    pew_tick();
    ++pewpew_ticks;

    // Clear the interrupt bit.
    tc->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;
}

void pew_init() {
    pew_obj_t *pew = MP_STATE_VM(pew_singleton);

    common_hal_digitalio_digitalinout_switch_to_input(pew->buttons, PULL_UP);

    for (size_t i = 0; i < pew->rows_size; ++i) {
        digitalio_digitalinout_obj_t *pin = MP_OBJ_TO_PTR(pew->rows[i]);
        common_hal_digitalio_digitalinout_switch_to_output(pin, false,
            DRIVE_MODE_PUSH_PULL);
    }
    for (size_t i = 0; i < pew->cols_size; ++i) {
        digitalio_digitalinout_obj_t *pin = MP_OBJ_TO_PTR(pew->cols[i]);
        common_hal_digitalio_digitalinout_switch_to_output(pin, true,
            DRIVE_MODE_OPEN_DRAIN);
    }
    if (pewpew_tc_index == 0xff) {
        // Find a spare timer.
        uint8_t index = find_free_timer();
        if (index == 0xff) {
            mp_raise_RuntimeError(MP_ERROR_TEXT("All timers in use"));
        }
        Tc *tc = tc_insts[index];

        pewpew_tc_index = index;
        set_timer_handler(true, index, TC_HANDLER_PEW);

        // We use GCLK0 for SAMD21 and GCLK1 for SAMD51 because they both run
        // at 48mhz making our math the same across the boards.
        #ifdef SAMD21
        turn_on_clocks(true, index, 0);
        #endif
        #ifdef SAM_D5X_E5X
        turn_on_clocks(true, index, 1);
        #endif


        #ifdef SAMD21
        tc->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 |
            TC_CTRLA_PRESCALER_DIV64 |
            TC_CTRLA_WAVEGEN_MFRQ;
        #endif
        #ifdef SAM_D5X_E5X
        tc_reset(tc);
        tc_set_enable(tc, false);
        tc->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16
            | TC_CTRLA_PRESCALER_DIV64;
        tc->COUNT16.WAVE.reg = TC_WAVE_WAVEGEN_MFRQ;
        #endif

        tc_set_enable(tc, true);
        tc->COUNT16.CC[0].reg = 64;

        // Clear our interrupt in case it was set earlier
        tc->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;
        tc->COUNT16.INTENSET.reg = TC_INTENSET_MC0;
        tc_enable_interrupts(pewpew_tc_index);
    }
}

void pew_reset(void) {
    if (pewpew_tc_index != 0xff) {
        tc_reset(tc_insts[pewpew_tc_index]);
        pewpew_tc_index = 0xff;
    }
    MP_STATE_VM(pew_singleton) = NULL;
}

uint16_t pew_get_ticks() {
    return pewpew_ticks;
}
