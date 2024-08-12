// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2016 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "reload.h"

#include "py/mphal.h"
#include "py/mpstate.h"
#include "supervisor/port.h"
#include "supervisor/shared/reload.h"
#include "supervisor/shared/tick.h"

#include "shared-bindings/supervisor/Runtime.h"

// True if user has disabled autoreload.
static bool autoreload_enabled = false;

// Non-zero if autoreload is temporarily off, due to an AUTORELOAD_SUSPEND_... reason.
static uint32_t autoreload_suspended = 0;

volatile uint32_t last_autoreload_trigger = 0;

void reload_initiate(supervisor_run_reason_t run_reason) {
    supervisor_set_run_reason(run_reason);

    // Raise reload exception, in case code is running.
    MP_STATE_THREAD(mp_pending_exception) = MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_reload_exception));
    #if MICROPY_ENABLE_SCHEDULER
    if (MP_STATE_VM(sched_state) == MP_SCHED_IDLE) {
        MP_STATE_VM(sched_state) = MP_SCHED_PENDING;
    }
    #endif
    port_wake_main_task();
}

void autoreload_reset() {
    last_autoreload_trigger = 0;
}

void autoreload_enable() {
    autoreload_enabled = true;
    last_autoreload_trigger = 0;
}

void autoreload_disable() {
    autoreload_enabled = false;
}

void autoreload_suspend(uint32_t suspend_reason_mask) {
    autoreload_suspended |= suspend_reason_mask;
}

void autoreload_resume(uint32_t suspend_reason_mask) {
    autoreload_suspended &= ~suspend_reason_mask;
}

inline bool autoreload_is_enabled() {
    return autoreload_enabled;
}

void autoreload_trigger() {
    if (!autoreload_enabled || autoreload_suspended != 0) {
        return;
    }
    bool reload_initiated = autoreload_pending();
    last_autoreload_trigger = supervisor_ticks_ms32();
    // Guard against the rare time that ticks is 0;
    if (last_autoreload_trigger == 0) {
        last_autoreload_trigger += 1;
    }
    // Initiate a reload of the VM immediately. Later code will pause to
    // wait for the autoreload to become ready. Doing the VM exit
    // immediately is clearer for the user.
    if (!reload_initiated) {
        reload_initiate(RUN_REASON_AUTO_RELOAD);
    }
}

bool autoreload_ready() {
    if (last_autoreload_trigger == 0 || autoreload_suspended != 0) {
        return false;
    }
    // Wait for autoreload interval before reloading
    uint32_t now = supervisor_ticks_ms32();
    uint32_t diff;
    if (now >= last_autoreload_trigger) {
        diff = now - last_autoreload_trigger;
    } else {
        diff = now + (0xffffffff - last_autoreload_trigger);
    }
    return diff > CIRCUITPY_AUTORELOAD_DELAY_MS;
}

bool autoreload_pending(void) {
    return last_autoreload_trigger > 0;
}
