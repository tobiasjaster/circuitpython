// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 Jeff Epler for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <math.h>

#include "py/runtime.h"

#include "shared-bindings/watchdog/__init__.h"
#include "shared-bindings/watchdog/WatchDogTimer.h"
#include "shared-bindings/microcontroller/__init__.h"

#include "common-hal/watchdog/WatchDogTimer.h"

#include "component/wdt.h"

#define SYNC_CTRL_WRITE while (WDT->SYNCBUSY.reg) {}

static void watchdog_disable(void) {
    // disable watchdog
    WDT->CTRLA.reg = 0;
    SYNC_CTRL_WRITE
}

static void watchdog_enable(watchdog_watchdogtimer_obj_t *self) {
    // disable watchdog for config
    watchdog_disable();

    int wdt_cycles = (int)(self->timeout * 1024);
    if (wdt_cycles < 8) {
        wdt_cycles = 8;
    }

    // ceil(log2(n)) = 32 - __builtin_clz(n - 1) when n > 1 (if int is 32 bits)
    int log2_wdt_cycles = (sizeof(int) * CHAR_BIT) - __builtin_clz(wdt_cycles - 1);
    int setting = log2_wdt_cycles - 3;      // CYC8_Val is 0

    OSC32KCTRL->OSCULP32K.bit.EN1K = 1;     // Enable out 1K (for WDT)

    WDT->INTENCLR.reg = WDT_INTENCLR_EW;    // Disable early warning interrupt
    WDT->CONFIG.bit.PER = setting;          // Set period for chip reset
    WDT->CTRLA.bit.WEN = 0;                 // Disable window mode
    SYNC_CTRL_WRITE
    common_hal_watchdog_feed(self);         // Clear watchdog interval
    WDT->CTRLA.bit.ENABLE = 1;              // Start watchdog now!
    SYNC_CTRL_WRITE
}

void common_hal_watchdog_feed(watchdog_watchdogtimer_obj_t *self) {
    WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;
}

void common_hal_watchdog_deinit(watchdog_watchdogtimer_obj_t *self) {
    if (self->mode == WATCHDOGMODE_NONE) {
        return;
    }
    watchdog_disable();
    self->mode = WATCHDOGMODE_NONE;
}

mp_float_t common_hal_watchdog_get_timeout(watchdog_watchdogtimer_obj_t *self) {
    return self->timeout;
}

void common_hal_watchdog_set_timeout(watchdog_watchdogtimer_obj_t *self, mp_float_t new_timeout) {
    mp_arg_validate_int_max(new_timeout, 16, MP_QSTR_timeout);
    self->timeout = new_timeout;

    if (self->mode == WATCHDOGMODE_RESET) {
        watchdog_enable(self);
    }
}

watchdog_watchdogmode_t common_hal_watchdog_get_mode(watchdog_watchdogtimer_obj_t *self) {
    return self->mode;
}

void common_hal_watchdog_set_mode(watchdog_watchdogtimer_obj_t *self, watchdog_watchdogmode_t new_mode) {
    if (self->mode == new_mode) {
        return;
    }

    switch (new_mode) {
        case WATCHDOGMODE_NONE:
            common_hal_watchdog_deinit(self);
            break;
        case WATCHDOGMODE_RAISE:
            mp_raise_NotImplementedError(NULL);
            break;
        case WATCHDOGMODE_RESET:
            watchdog_enable(self);
            break;
        default:
            return;
    }

    self->mode = new_mode;
}
