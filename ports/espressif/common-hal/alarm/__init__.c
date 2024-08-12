// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Scott Shawcroft for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2020 Dan Halbert for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "py/gc.h"
#include "py/obj.h"
#include "py/objtuple.h"
#include "py/runtime.h"

#include "shared-bindings/alarm/__init__.h"
#include "shared-bindings/alarm/SleepMemory.h"
#include "shared-bindings/alarm/pin/PinAlarm.h"
#include "shared-bindings/alarm/time/TimeAlarm.h"
#include "shared-bindings/alarm/touch/TouchAlarm.h"

#include "shared-bindings/wifi/__init__.h"
#include "shared-bindings/microcontroller/__init__.h"

#if CIRCUITPY_ESPULP
#include "bindings/espulp/ULPAlarm.h"
#endif

#include "common-hal/digitalio/DigitalInOut.h"

#include "supervisor/port.h"
#include "supervisor/shared/workflow.h"

#include "esp_sleep.h"

#include "components/driver/gpio/include/driver/gpio.h"
#include "components/driver/uart/include/driver/uart.h"

// Singleton instance of SleepMemory.
const alarm_sleep_memory_obj_t alarm_sleep_memory_obj = {
    .base = {
        .type = &alarm_sleep_memory_type,
    },
};

// Non-heap alarm object recording alarm (if any) that woke up CircuitPython after light or deep sleep.
// This object lives across VM instantiations, so none of these objects can contain references to the heap.
alarm_wake_alarm_union_t alarm_wake_alarm;

void alarm_reset(void) {
    alarm_sleep_memory_reset();
    alarm_pin_pinalarm_reset();
    alarm_time_timealarm_reset();
    #if CIRCUITPY_ALARM_TOUCH
    alarm_touch_touchalarm_reset();
    #endif
    #if CIRCUITPY_ESPULP
    espulp_ulpalarm_reset();
    #endif
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
}

static esp_sleep_wakeup_cause_t _get_wakeup_cause(bool deep_sleep) {
    // First check if the modules remember what last woke up
    if (alarm_pin_pinalarm_woke_this_cycle()) {
        return ESP_SLEEP_WAKEUP_GPIO;
    }
    if (alarm_time_timealarm_woke_this_cycle()) {
        return ESP_SLEEP_WAKEUP_TIMER;
    }
    #if CIRCUITPY_ALARM_TOUCH
    if (alarm_touch_touchalarm_woke_this_cycle()) {
        return ESP_SLEEP_WAKEUP_TOUCHPAD;
    }
    #endif
    #if CIRCUITPY_ESPULP
    if (espulp_ulpalarm_woke_this_cycle()) {
        return ESP_SLEEP_WAKEUP_ULP;
    }
    #endif
    // If waking from true deep sleep, modules will have lost their state,
    // so check the deep wakeup cause manually
    if (deep_sleep) {
        return esp_sleep_get_wakeup_cause();
    }
    return ESP_SLEEP_WAKEUP_UNDEFINED;
}

bool common_hal_alarm_woken_from_sleep(void) {
    return _get_wakeup_cause(false) != ESP_SLEEP_WAKEUP_UNDEFINED;
}

mp_obj_t common_hal_alarm_record_wake_alarm(void) {
    // If woken from deep sleep, create a copy alarm similar to what would have
    // been passed in originally. Otherwise, just return none
    esp_sleep_wakeup_cause_t cause = _get_wakeup_cause(true);
    switch (cause) {
        case ESP_SLEEP_WAKEUP_TIMER: {
            return alarm_time_timealarm_record_wake_alarm();
        }

        case ESP_SLEEP_WAKEUP_GPIO:
        case ESP_SLEEP_WAKEUP_EXT0:
        case ESP_SLEEP_WAKEUP_EXT1: {
            return alarm_pin_pinalarm_record_wake_alarm();
        }

        #if CIRCUITPY_ALARM_TOUCH
        case ESP_SLEEP_WAKEUP_TOUCHPAD: {
            return alarm_touch_touchalarm_record_wake_alarm();
        }
        #endif

        #if CIRCUITPY_ESPULP
        case ESP_SLEEP_WAKEUP_ULP: {
            return espulp_ulpalarm_record_wake_alarm();
        }
        #endif

        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            // Not a deep sleep reset.
            break;
    }
    return mp_const_none;
}

// Set up light sleep or deep sleep alarms.
static void _setup_sleep_alarms(bool deep_sleep, size_t n_alarms, const mp_obj_t *alarms) {
    alarm_pin_pinalarm_set_alarms(deep_sleep, n_alarms, alarms);
    alarm_time_timealarm_set_alarms(deep_sleep, n_alarms, alarms);
    #if CIRCUITPY_ALARM_TOUCH
    alarm_touch_touchalarm_set_alarm(deep_sleep, n_alarms, alarms);
    #endif
    #if CIRCUITPY_ESPULP
    espulp_ulpalarm_set_alarm(deep_sleep, n_alarms, alarms);
    #endif
}

mp_obj_t common_hal_alarm_light_sleep_until_alarms(size_t n_alarms, const mp_obj_t *alarms) {
    _setup_sleep_alarms(false, n_alarms, alarms);

    mp_obj_t wake_alarm = mp_const_none;

    // We cannot esp_light_sleep_start() here because it shuts down all non-RTC peripherals.
    while (!mp_hal_is_interrupted()) {
        RUN_BACKGROUND_TASKS;
        // Detect if interrupt was alarm or ctrl-C interrupt.
        if (common_hal_alarm_woken_from_sleep()) {
            esp_sleep_wakeup_cause_t cause = _get_wakeup_cause(false);
            switch (cause) {
                case ESP_SLEEP_WAKEUP_TIMER: {
                    wake_alarm = alarm_time_timealarm_find_triggered_alarm(n_alarms, alarms);
                    break;
                }
                case ESP_SLEEP_WAKEUP_GPIO: {
                    wake_alarm = alarm_pin_pinalarm_find_triggered_alarm(n_alarms, alarms);
                    break;
                }
                #if CIRCUITPY_ALARM_TOUCH
                case ESP_SLEEP_WAKEUP_TOUCHPAD: {
                    wake_alarm = alarm_touch_touchalarm_find_triggered_alarm(n_alarms, alarms);
                    break;
                }
                #endif
                #if CIRCUITPY_ESPULP
                case ESP_SLEEP_WAKEUP_ULP: {
                    wake_alarm = espulp_ulpalarm_find_triggered_alarm(n_alarms, alarms);
                    break;
                }
                #endif
                default:
                    // Should not reach this, if all light sleep types are covered correctly
                    break;
            }
            shared_alarm_save_wake_alarm(wake_alarm);
            break;
        }
        port_idle_until_interrupt();
    }

    if (mp_hal_is_interrupted()) {
        return mp_const_none; // Shouldn't be given to python code because exception handling should kick in.
    }

    alarm_reset();
    return wake_alarm;
}

void common_hal_alarm_set_deep_sleep_alarms(size_t n_alarms, const mp_obj_t *alarms, size_t n_dios, digitalio_digitalinout_obj_t *preserve_dios[]) {
    digitalio_digitalinout_preserve_for_deep_sleep(n_dios, preserve_dios);
    _setup_sleep_alarms(true, n_alarms, alarms);
}

void NORETURN common_hal_alarm_enter_deep_sleep(void) {
    alarm_pin_pinalarm_prepare_for_deep_sleep();
    #if CIRCUITPY_ALARM_TOUCH
    alarm_touch_touchalarm_prepare_for_deep_sleep();
    #endif
    #if CIRCUITPY_ESPULP
    espulp_ulpalarm_prepare_for_deep_sleep();
    #endif

    // We no longer need to remember the pin preservations, since any pin resets are all done.
    clear_pin_preservations();

    // The ESP-IDF caches the deep sleep settings and applies them before sleep.
    // We don't need to worry about resetting them in the interim.
    esp_deep_sleep_start();

    reset_into_safe_mode(SAFE_MODE_HARD_FAULT);
}

void common_hal_alarm_gc_collect(void) {
    gc_collect_ptr(shared_alarm_get_wake_alarm());
}
