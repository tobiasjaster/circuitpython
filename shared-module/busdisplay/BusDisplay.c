// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2018 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/busdisplay/BusDisplay.h"

#include "py/runtime.h"
#if CIRCUITPY_FOURWIRE
#include "shared-bindings/fourwire/FourWire.h"
#endif
#if CIRCUITPY_I2CDISPLAYBUS
#include "shared-bindings/i2cdisplaybus/I2CDisplayBus.h"
#endif
#if CIRCUITPY_PARALLELDISPLAYBUS
#include "shared-bindings/paralleldisplaybus/ParallelBus.h"
#endif
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/time/__init__.h"
#include "shared-module/displayio/__init__.h"
#include "shared-module/displayio/display_core.h"
#include "supervisor/shared/display.h"
#include "supervisor/shared/tick.h"

#if CIRCUITPY_TINYUSB
#include "supervisor/usb.h"
#endif

#include <stdint.h>
#include <string.h>

#define DELAY 0x80

void common_hal_busdisplay_busdisplay_construct(busdisplay_busdisplay_obj_t *self,
    mp_obj_t bus, uint16_t width, uint16_t height, int16_t colstart, int16_t rowstart,
    uint16_t rotation, uint16_t color_depth, bool grayscale, bool pixels_in_byte_share_row,
    uint8_t bytes_per_cell, bool reverse_pixels_in_byte, bool reverse_bytes_in_word, uint8_t set_column_command,
    uint8_t set_row_command, uint8_t write_ram_command,
    uint8_t *init_sequence, uint16_t init_sequence_len, const mcu_pin_obj_t *backlight_pin,
    uint16_t brightness_command, mp_float_t brightness,
    bool single_byte_bounds, bool data_as_commands, bool auto_refresh, uint16_t native_frames_per_second,
    bool backlight_on_high, bool SH1107_addressing, uint16_t backlight_pwm_frequency) {

    // Turn off auto-refresh as we init.
    self->auto_refresh = false;
    uint16_t ram_width = 0x100;
    uint16_t ram_height = 0x100;
    if (single_byte_bounds) {
        ram_width = 0xff;
        ram_height = 0xff;
    }
    displayio_display_core_construct(&self->core, width, height, rotation,
        color_depth, grayscale, pixels_in_byte_share_row, bytes_per_cell, reverse_pixels_in_byte, reverse_bytes_in_word);
    displayio_display_bus_construct(&self->bus, bus, ram_width, ram_height, colstart, rowstart,
        set_column_command, set_row_command, NO_COMMAND, NO_COMMAND, data_as_commands, false /* always_toggle_chip_select */,
        SH1107_addressing && color_depth == 1, false /*address_little_endian */);

    self->write_ram_command = write_ram_command;
    self->brightness_command = brightness_command;
    self->first_manual_refresh = !auto_refresh;
    self->backlight_on_high = backlight_on_high;

    self->native_frames_per_second = native_frames_per_second;
    self->native_ms_per_frame = 1000 / native_frames_per_second;

    uint32_t i = 0;
    while (i < init_sequence_len) {
        uint8_t *cmd = init_sequence + i;
        uint8_t data_size = *(cmd + 1);
        bool delay = (data_size & DELAY) != 0;
        data_size &= ~DELAY;
        uint8_t *data = cmd + 2;
        while (!displayio_display_bus_begin_transaction(&self->bus)) {
            RUN_BACKGROUND_TASKS;
        }
        if (self->bus.data_as_commands) {
            uint8_t full_command[data_size + 1];
            full_command[0] = cmd[0];
            memcpy(full_command + 1, data, data_size);
            self->bus.send(self->bus.bus, DISPLAY_COMMAND, CHIP_SELECT_TOGGLE_EVERY_BYTE, full_command, data_size + 1);
        } else {
            self->bus.send(self->bus.bus, DISPLAY_COMMAND, CHIP_SELECT_TOGGLE_EVERY_BYTE, cmd, 1);
            self->bus.send(self->bus.bus, DISPLAY_DATA, CHIP_SELECT_UNTOUCHED, data, data_size);
        }
        displayio_display_bus_end_transaction(&self->bus);
        uint16_t delay_length_ms = 10;
        if (delay) {
            data_size++;
            delay_length_ms = *(cmd + 1 + data_size);
            if (delay_length_ms == 255) {
                delay_length_ms = 500;
            }
        }
        common_hal_time_delay_ms(delay_length_ms);
        i += 2 + data_size;
    }

    // Always set the backlight type in case we're reusing memory.
    self->backlight_inout.base.type = &mp_type_NoneType;
    if (backlight_pin != NULL && common_hal_mcu_pin_is_free(backlight_pin)) {
        // Avoid PWM types and functions when the module isn't enabled
        #if (CIRCUITPY_PWMIO)
        pwmout_result_t result = common_hal_pwmio_pwmout_construct(&self->backlight_pwm, backlight_pin, 0, backlight_pwm_frequency, false);
        if (result != PWMOUT_OK) {
            self->backlight_inout.base.type = &digitalio_digitalinout_type;
            common_hal_digitalio_digitalinout_construct(&self->backlight_inout, backlight_pin);
            common_hal_never_reset_pin(backlight_pin);
        } else {
            self->backlight_pwm.base.type = &pwmio_pwmout_type;
            common_hal_pwmio_pwmout_never_reset(&self->backlight_pwm);
        }
        #else
        // Otherwise default to digital
        self->backlight_inout.base.type = &digitalio_digitalinout_type;
        common_hal_digitalio_digitalinout_construct(&self->backlight_inout, backlight_pin);
        common_hal_never_reset_pin(backlight_pin);
        #endif
    }

    common_hal_busdisplay_busdisplay_set_brightness(self, brightness);

    // Set the group after initialization otherwise we may send pixels while we delay in
    // initialization.
    if (!circuitpython_splash.in_group) {
        common_hal_busdisplay_busdisplay_set_root_group(self, &circuitpython_splash);
    }
    common_hal_busdisplay_busdisplay_set_auto_refresh(self, auto_refresh);
}

uint16_t common_hal_busdisplay_busdisplay_get_width(busdisplay_busdisplay_obj_t *self) {
    return displayio_display_core_get_width(&self->core);
}

uint16_t common_hal_busdisplay_busdisplay_get_height(busdisplay_busdisplay_obj_t *self) {
    return displayio_display_core_get_height(&self->core);
}

mp_float_t common_hal_busdisplay_busdisplay_get_brightness(busdisplay_busdisplay_obj_t *self) {
    return self->current_brightness;
}

bool common_hal_busdisplay_busdisplay_set_brightness(busdisplay_busdisplay_obj_t *self, mp_float_t brightness) {
    if (!self->backlight_on_high) {
        brightness = 1.0 - brightness;
    }
    bool ok = false;

    // Avoid PWM types and functions when the module isn't enabled
    #if (CIRCUITPY_PWMIO)
    bool ispwm = (self->backlight_pwm.base.type == &pwmio_pwmout_type) ? true : false;
    #else
    bool ispwm = false;
    #endif

    if (ispwm) {
        #if (CIRCUITPY_PWMIO)
        common_hal_pwmio_pwmout_set_duty_cycle(&self->backlight_pwm, (uint16_t)(0xffff * brightness));
        ok = true;
        #else
        ok = false;
        #endif
    } else if (self->backlight_inout.base.type == &digitalio_digitalinout_type) {
        common_hal_digitalio_digitalinout_set_value(&self->backlight_inout, brightness > 0.99);
        ok = true;
    } else if (self->brightness_command != NO_BRIGHTNESS_COMMAND) {
        ok = displayio_display_bus_begin_transaction(&self->bus);
        if (ok) {
            if (self->bus.data_as_commands) {
                uint8_t set_brightness[2] = {self->brightness_command, (uint8_t)(0xff * brightness)};
                self->bus.send(self->bus.bus, DISPLAY_COMMAND, CHIP_SELECT_TOGGLE_EVERY_BYTE, set_brightness, 2);
            } else {
                uint8_t command = self->brightness_command;
                uint8_t hex_brightness = 0xff * brightness;
                self->bus.send(self->bus.bus, DISPLAY_COMMAND, CHIP_SELECT_TOGGLE_EVERY_BYTE, &command, 1);
                self->bus.send(self->bus.bus, DISPLAY_DATA, CHIP_SELECT_UNTOUCHED, &hex_brightness, 1);
            }
            displayio_display_bus_end_transaction(&self->bus);
        }

    }
    if (ok) {
        self->current_brightness = brightness;
    }
    return ok;
}

mp_obj_t common_hal_busdisplay_busdisplay_get_bus(busdisplay_busdisplay_obj_t *self) {
    return self->bus.bus;
}

mp_obj_t common_hal_busdisplay_busdisplay_get_root_group(busdisplay_busdisplay_obj_t *self) {
    if (self->core.current_group == NULL) {
        return mp_const_none;
    }
    return self->core.current_group;
}

static const displayio_area_t *_get_refresh_areas(busdisplay_busdisplay_obj_t *self) {
    if (self->core.full_refresh) {
        self->core.area.next = NULL;
        return &self->core.area;
    } else if (self->core.current_group != NULL) {
        return displayio_group_get_refresh_areas(self->core.current_group, NULL);
    }
    return NULL;
}

static void _send_pixels(busdisplay_busdisplay_obj_t *self, uint8_t *pixels, uint32_t length) {
    if (!self->bus.data_as_commands) {
        self->bus.send(self->bus.bus, DISPLAY_COMMAND, CHIP_SELECT_TOGGLE_EVERY_BYTE, &self->write_ram_command, 1);
    }
    self->bus.send(self->bus.bus, DISPLAY_DATA, CHIP_SELECT_UNTOUCHED, pixels, length);
}

static bool _refresh_area(busdisplay_busdisplay_obj_t *self, const displayio_area_t *area) {
    uint16_t buffer_size = 128; // In uint32_ts

    displayio_area_t clipped;
    // Clip the area to the display by overlapping the areas. If there is no overlap then we're done.
    if (!displayio_display_core_clip_area(&self->core, area, &clipped)) {
        return true;
    }
    uint16_t rows_per_buffer = displayio_area_height(&clipped);
    uint8_t pixels_per_word = (sizeof(uint32_t) * 8) / self->core.colorspace.depth;
    uint16_t pixels_per_buffer = displayio_area_size(&clipped);

    uint16_t subrectangles = 1;
    // for SH1107 and other boundary constrained controllers
    //      write one single row at a time
    if (self->bus.SH1107_addressing) {
        subrectangles = rows_per_buffer / 8;  // page addressing mode writes 8 rows at a time
        rows_per_buffer = 8;
    } else if (displayio_area_size(&clipped) > buffer_size * pixels_per_word) {
        rows_per_buffer = buffer_size * pixels_per_word / displayio_area_width(&clipped);
        if (rows_per_buffer == 0) {
            rows_per_buffer = 1;
        }
        // If pixels are packed by column then ensure rows_per_buffer is on a byte boundary.
        if (self->core.colorspace.depth < 8 && !self->core.colorspace.pixels_in_byte_share_row) {
            uint8_t pixels_per_byte = 8 / self->core.colorspace.depth;
            if (rows_per_buffer % pixels_per_byte != 0) {
                rows_per_buffer -= rows_per_buffer % pixels_per_byte;
            }
        }
        subrectangles = displayio_area_height(&clipped) / rows_per_buffer;
        if (displayio_area_height(&clipped) % rows_per_buffer != 0) {
            subrectangles++;
        }
        pixels_per_buffer = rows_per_buffer * displayio_area_width(&clipped);
        buffer_size = pixels_per_buffer / pixels_per_word;
        if (pixels_per_buffer % pixels_per_word) {
            buffer_size += 1;
        }
    }

    // Allocated and shared as a uint32_t array so the compiler knows the
    // alignment everywhere.
    uint32_t buffer[buffer_size];
    uint32_t mask_length = (pixels_per_buffer / 32) + 1;
    uint32_t mask[mask_length];
    uint16_t remaining_rows = displayio_area_height(&clipped);

    for (uint16_t j = 0; j < subrectangles; j++) {
        displayio_area_t subrectangle = {
            .x1 = clipped.x1,
            .y1 = clipped.y1 + rows_per_buffer * j,
            .x2 = clipped.x2,
            .y2 = clipped.y1 + rows_per_buffer * (j + 1)
        };
        if (remaining_rows < rows_per_buffer) {
            subrectangle.y2 = subrectangle.y1 + remaining_rows;
        }
        remaining_rows -= rows_per_buffer;

        displayio_display_bus_set_region_to_update(&self->bus, &self->core, &subrectangle);

        uint16_t subrectangle_size_bytes;
        if (self->core.colorspace.depth >= 8) {
            subrectangle_size_bytes = displayio_area_size(&subrectangle) * (self->core.colorspace.depth / 8);
        } else {
            subrectangle_size_bytes = displayio_area_size(&subrectangle) / (8 / self->core.colorspace.depth);
        }

        memset(mask, 0, mask_length * sizeof(mask[0]));
        memset(buffer, 0, buffer_size * sizeof(buffer[0]));

        displayio_display_core_fill_area(&self->core, &subrectangle, mask, buffer);

        // Can't acquire display bus; skip the rest of the data.
        if (!displayio_display_bus_is_free(&self->bus)) {
            return false;
        }

        displayio_display_bus_begin_transaction(&self->bus);
        _send_pixels(self, (uint8_t *)buffer, subrectangle_size_bytes);
        displayio_display_bus_end_transaction(&self->bus);

        // TODO(tannewt): Make refresh displays faster so we don't starve other
        // background tasks.
        #if CIRCUITPY_TINYUSB
        usb_background();
        #endif
    }
    return true;
}

static void _refresh_display(busdisplay_busdisplay_obj_t *self) {
    if (!displayio_display_bus_is_free(&self->bus)) {
        // A refresh on this bus is already in progress.  Try next display.
        return;
    }
    displayio_display_core_start_refresh(&self->core);
    const displayio_area_t *current_area = _get_refresh_areas(self);
    while (current_area != NULL) {
        _refresh_area(self, current_area);
        current_area = current_area->next;
    }
    displayio_display_core_finish_refresh(&self->core);
}

void common_hal_busdisplay_busdisplay_set_rotation(busdisplay_busdisplay_obj_t *self, int rotation) {
    bool transposed = (self->core.rotation == 90 || self->core.rotation == 270);
    bool will_transposed = (rotation == 90 || rotation == 270);
    if (transposed != will_transposed) {
        int tmp = self->core.width;
        self->core.width = self->core.height;
        self->core.height = tmp;
    }
    displayio_display_core_set_rotation(&self->core, rotation);
    if (self == &displays[0].display) {
        supervisor_stop_terminal();
        supervisor_start_terminal(self->core.width, self->core.height);
    }
    if (self->core.current_group != NULL) {
        displayio_group_update_transform(self->core.current_group, &self->core.transform);
    }
}

uint16_t common_hal_busdisplay_busdisplay_get_rotation(busdisplay_busdisplay_obj_t *self) {
    return self->core.rotation;
}


bool common_hal_busdisplay_busdisplay_refresh(busdisplay_busdisplay_obj_t *self, uint32_t target_ms_per_frame, uint32_t maximum_ms_per_real_frame) {
    if (!self->auto_refresh && !self->first_manual_refresh && (target_ms_per_frame != NO_FPS_LIMIT)) {
        uint64_t current_time = supervisor_ticks_ms64();
        uint32_t current_ms_since_real_refresh = current_time - self->core.last_refresh;
        // Test to see if the real frame time is below our minimum.
        if (current_ms_since_real_refresh > maximum_ms_per_real_frame) {
            mp_raise_RuntimeError(MP_ERROR_TEXT("Below minimum frame rate"));
        }
        uint32_t current_ms_since_last_call = current_time - self->last_refresh_call;
        self->last_refresh_call = current_time;
        // Skip the actual refresh to help catch up.
        if (current_ms_since_last_call > target_ms_per_frame) {
            return false;
        }
        uint32_t remaining_time = target_ms_per_frame - (current_ms_since_real_refresh % target_ms_per_frame);
        // We're ahead of the game so wait until we align with the frame rate.
        while (supervisor_ticks_ms64() - self->last_refresh_call < remaining_time) {
            RUN_BACKGROUND_TASKS;
        }
    }
    self->first_manual_refresh = false;
    _refresh_display(self);
    return true;
}

bool common_hal_busdisplay_busdisplay_get_auto_refresh(busdisplay_busdisplay_obj_t *self) {
    return self->auto_refresh;
}

void common_hal_busdisplay_busdisplay_set_auto_refresh(busdisplay_busdisplay_obj_t *self,
    bool auto_refresh) {
    self->first_manual_refresh = !auto_refresh;
    if (auto_refresh != self->auto_refresh) {
        if (auto_refresh) {
            supervisor_enable_tick();
        } else {
            supervisor_disable_tick();
        }
    }
    self->auto_refresh = auto_refresh;
}

mp_obj_t common_hal_busdisplay_busdisplay_set_root_group(busdisplay_busdisplay_obj_t *self, displayio_group_t *root_group) {
    bool ok = displayio_display_core_set_root_group(&self->core, root_group);
    if (!ok) {
        mp_raise_ValueError(MP_ERROR_TEXT("Group already used"));
    }
    return mp_const_none;
}

void busdisplay_busdisplay_background(busdisplay_busdisplay_obj_t *self) {
    if (self->auto_refresh && (supervisor_ticks_ms64() - self->core.last_refresh) > self->native_ms_per_frame) {
        _refresh_display(self);
    }
}

void release_busdisplay(busdisplay_busdisplay_obj_t *self) {
    common_hal_busdisplay_busdisplay_set_auto_refresh(self, false);
    release_display_core(&self->core);
    #if (CIRCUITPY_PWMIO)
    if (self->backlight_pwm.base.type == &pwmio_pwmout_type) {
        common_hal_pwmio_pwmout_deinit(&self->backlight_pwm);
    } else if (self->backlight_inout.base.type == &digitalio_digitalinout_type) {
        common_hal_digitalio_digitalinout_deinit(&self->backlight_inout);
    }
    #else
    common_hal_digitalio_digitalinout_deinit(&self->backlight_inout);
    #endif
}

void reset_busdisplay(busdisplay_busdisplay_obj_t *self) {
    common_hal_busdisplay_busdisplay_set_auto_refresh(self, true);
    circuitpython_splash.x = 0; // reset position in case someone moved it.
    circuitpython_splash.y = 0;
    supervisor_start_terminal(self->core.width, self->core.height);
    if (!circuitpython_splash.in_group) {
        common_hal_busdisplay_busdisplay_set_root_group(self, &circuitpython_splash);
    }
}

void busdisplay_busdisplay_collect_ptrs(busdisplay_busdisplay_obj_t *self) {
    displayio_display_core_collect_ptrs(&self->core);
    displayio_display_bus_collect_ptrs(&self->bus);
}
