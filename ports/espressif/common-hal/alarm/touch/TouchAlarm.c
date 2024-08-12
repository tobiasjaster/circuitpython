// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 microDev
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/alarm/__init__.h"
#include "shared-bindings/alarm/touch/TouchAlarm.h"
#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/microcontroller/Pin.h"

#include "esp_sleep.h"
#include "peripherals/touch.h"
#include "supervisor/port.h"

static uint16_t touch_channel_mask;
static volatile bool woke_up = false;

void common_hal_alarm_touch_touchalarm_construct(alarm_touch_touchalarm_obj_t *self, const mcu_pin_obj_t *pin) {
    if (pin->touch_channel == TOUCH_PAD_MAX) {
        raise_ValueError_invalid_pin();
    }
    claim_pin(pin);
    self->pin = pin;
}

// Used for light sleep.
mp_obj_t alarm_touch_touchalarm_find_triggered_alarm(const size_t n_alarms, const mp_obj_t *alarms) {
    for (size_t i = 0; i < n_alarms; i++) {
        if (mp_obj_is_type(alarms[i], &alarm_touch_touchalarm_type)) {
            return alarms[i];
        }
    }
    return mp_const_none;
}

mp_obj_t alarm_touch_touchalarm_record_wake_alarm(void) {
    alarm_touch_touchalarm_obj_t *const alarm = &alarm_wake_alarm.touch_alarm;

    alarm->base.type = &alarm_touch_touchalarm_type;
    alarm->pin = NULL;

    #if defined(CONFIG_IDF_TARGET_ESP32)
    touch_pad_t wake_channel;
    if (touch_pad_get_wakeup_status(&wake_channel) != ESP_OK) {
        return alarm;
    }
    #else
    touch_pad_t wake_channel = touch_pad_get_current_meas_channel();
    if (wake_channel == TOUCH_PAD_MAX) {
        return alarm;
    }
    #endif

    // Map the pin number back to a pin object.
    for (size_t i = 0; i < mcu_pin_globals.map.used; i++) {
        const mcu_pin_obj_t *pin_obj = MP_OBJ_TO_PTR(mcu_pin_globals.map.table[i].value);
        if (pin_obj->touch_channel == wake_channel) {
            alarm->pin = mcu_pin_globals.map.table[i].value;
            break;
        }
    }

    return alarm;
}

// This is used to wake the main CircuitPython task.
static void touch_interrupt(void *arg) {
    (void)arg;
    woke_up = true;
    port_wake_main_task_from_isr();
}

void alarm_touch_touchalarm_set_alarm(const bool deep_sleep, const size_t n_alarms, const mp_obj_t *alarms) {
    bool touch_alarm_set = false;
    alarm_touch_touchalarm_obj_t *touch_alarm = MP_OBJ_NULL;

    for (size_t i = 0; i < n_alarms; i++) {
        if (mp_obj_is_type(alarms[i], &alarm_touch_touchalarm_type)) {
            if (deep_sleep && touch_alarm_set) {
                mp_raise_ValueError_varg(MP_ERROR_TEXT("Only one %q can be set in deep sleep."), MP_QSTR_TouchAlarm);
            }
            touch_alarm = MP_OBJ_TO_PTR(alarms[i]);
            touch_channel_mask |= 1 << touch_alarm->pin->number;
            // Resetting the pin will set a pull-up, which we don't want.
            skip_reset_once_pin_number(touch_alarm->pin->number);
            touch_alarm_set = true;
        }
    }

    if (!touch_alarm_set) {
        return;
    }

    // configure interrupt for pretend to deep sleep
    // this will be disabled if we actually deep sleep

    // reset touch peripheral
    peripherals_touch_reset();
    peripherals_touch_never_reset(true);

    for (uint8_t i = 1; i <= 14; i++) {
        if ((touch_channel_mask & 1 << i) != 0) {
            touch_pad_t touch_channel = (touch_pad_t)i;
            // initialize touchpad
            peripherals_touch_init(touch_channel);

            // wait for touch data to reset
            mp_hal_delay_ms(10);

            // configure trigger threshold
            #if defined(CONFIG_IDF_TARGET_ESP32)
            uint16_t touch_value;
            // ESP32 touch_pad_read() returns a lower value when a pin is touched, not a higher value
            // Typical values on a Feather ESP32 V2 are 600 with a short jumper untouched,
            // 70 touched.
            touch_pad_read(touch_channel, &touch_value);
            touch_pad_set_thresh(touch_channel, touch_value / 2);
            #else
            uint32_t touch_value;
            touch_pad_read_benchmark(touch_channel, &touch_value);
            touch_pad_set_thresh(touch_channel, touch_value / 10); // 10%
            #endif
        }
    }

    // configure touch interrupt
    #if defined(CONFIG_IDF_TARGET_ESP32)
    touch_pad_isr_register(touch_interrupt, NULL);
    touch_pad_intr_enable();
    #else
    touch_pad_timeout_set(true, SOC_TOUCH_PAD_THRESHOLD_MAX);
    touch_pad_isr_register(touch_interrupt, NULL, TOUCH_PAD_INTR_MASK_ALL);
    touch_pad_intr_enable(TOUCH_PAD_INTR_MASK_ACTIVE | TOUCH_PAD_INTR_MASK_INACTIVE);
    #endif
}

void alarm_touch_touchalarm_prepare_for_deep_sleep(void) {
    if (!touch_channel_mask) {
        return;
    }

    touch_pad_t touch_channel = TOUCH_PAD_MAX;
    for (uint8_t i = 1; i <= 14; i++) {
        if ((touch_channel_mask & 1 << i) != 0) {
            touch_channel = (touch_pad_t)i;
            break;
        }
    }

    // reset touch peripheral
    peripherals_touch_never_reset(false);
    peripherals_touch_reset();

    // initialize touchpad
    peripherals_touch_init(touch_channel);

    #if !defined(CONFIG_IDF_TARGET_ESP32)
    // configure touchpad for sleep
    touch_pad_sleep_channel_enable(touch_channel, true);
    touch_pad_sleep_channel_enable_proximity(touch_channel, false);
    #endif

    // wait for touch data to reset
    mp_hal_delay_ms(10);

    // configure trigger threshold
    #if defined(CONFIG_IDF_TARGET_ESP32)
    uint16_t touch_value;
    touch_pad_read(touch_channel, &touch_value);
    // ESP32 touch_pad_read() returns a lower value when a pin is touched, not a higher value
    // Typical values on a Feather ESP32 V2 are 600 with a short jumper untouched,
    // 70 touched.
    touch_pad_set_thresh(touch_channel, touch_value / 2);
    #else
    uint32_t touch_value;
    touch_pad_sleep_channel_read_smooth(touch_channel, &touch_value);
    touch_pad_sleep_set_threshold(touch_channel, touch_value / 10); // 10%
    #endif

    // enable touchpad wakeup
    esp_sleep_enable_touchpad_wakeup();
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
}

bool alarm_touch_touchalarm_woke_this_cycle(void) {
    return woke_up;
}

void alarm_touch_touchalarm_reset(void) {
    woke_up = false;
    touch_channel_mask = 0;
    peripherals_touch_never_reset(false);
}
