// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Nick Moore for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <stdlib.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "shared/timeutils/timeutils.h"
#include "shared-bindings/rtc/__init__.h"
#include "common-hal/rtc/RTC.h"
#include "shared-bindings/rtc/RTC.h"
#include "supervisor/port.h"

// This is the time in seconds since 2000 that the RTC was started.
__attribute__((section(".uninitialized"))) static uint32_t rtc_offset[3];

// These values are placed before and after the current RTC count.  They are
// used to determine if the RTC count is valid.  These randomly-generated values
// will be set when the RTC value is set in order to mark the RTC as valid. If
// the system crashes or reboots, these values will remain undisturbed and the
// RTC offset will remain valid.
//
// If CircuitPython is updated or these symbols shift around, the prefix and
// suffix will no longer match, and the time will no longer be valid.
#define RTC_OFFSET_CHECK_PREFIX 0x25ea7e2a
#define RTC_OFFSET_CHECK_SUFFIX 0x2b80b69e

void common_hal_rtc_init(void) {
    // If the prefix and suffix are not valid, zero-initialize the RTC offset.
    if ((rtc_offset[0] != RTC_OFFSET_CHECK_PREFIX) || (rtc_offset[2] != RTC_OFFSET_CHECK_SUFFIX)) {
        rtc_offset[1] = 0;
    }
}

void common_hal_rtc_get_time(timeutils_struct_time_t *tm) {
    uint64_t ticks_s = port_get_raw_ticks(NULL) / 1024;
    timeutils_seconds_since_2000_to_struct_time(rtc_offset[1] + ticks_s, tm);
}

void common_hal_rtc_set_time(timeutils_struct_time_t *tm) {
    uint64_t ticks_s = port_get_raw_ticks(NULL) / 1024;
    uint32_t epoch_s = timeutils_seconds_since_2000(
        tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec
        );
    rtc_offset[1] = epoch_s - ticks_s;

    // Set the prefix and suffix in order to indicate the time is valid.  This
    // must be done after the offset is updated, in case there is a crash or
    // power failure.
    rtc_offset[0] = RTC_OFFSET_CHECK_PREFIX;
    rtc_offset[2] = RTC_OFFSET_CHECK_SUFFIX;
}

int common_hal_rtc_get_calibration(void) {
    return 0;
}

void common_hal_rtc_set_calibration(int calibration) {
    mp_raise_NotImplementedError_varg(MP_ERROR_TEXT("%q"), MP_QSTR_calibration);
}
