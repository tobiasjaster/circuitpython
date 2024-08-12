// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/epaperdisplay/EPaperDisplay.h"

#include "py/gc.h"
#include "py/runtime.h"
#include "shared/runtime/interrupt_char.h"
#include "shared-bindings/displayio/ColorConverter.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/time/__init__.h"
#include "shared-module/displayio/__init__.h"
#include "supervisor/shared/display.h"
#include "supervisor/shared/tick.h"

#if CIRCUITPY_TINYUSB
#include "supervisor/usb.h"
#endif

#include <stdint.h>
#include <string.h>

#define DELAY 0x80

void common_hal_epaperdisplay_epaperdisplay_construct(epaperdisplay_epaperdisplay_obj_t *self,
    mp_obj_t bus, const uint8_t *start_sequence, uint16_t start_sequence_len, mp_float_t start_up_time,
    const uint8_t *stop_sequence, uint16_t stop_sequence_len,
    uint16_t width, uint16_t height, uint16_t ram_width, uint16_t ram_height,
    int16_t colstart, int16_t rowstart, uint16_t rotation,
    uint16_t set_column_window_command, uint16_t set_row_window_command,
    uint16_t set_current_column_command, uint16_t set_current_row_command,
    uint16_t write_black_ram_command, bool black_bits_inverted,
    uint16_t write_color_ram_command, bool color_bits_inverted, uint32_t highlight_color,
    const uint8_t *refresh_sequence, uint16_t refresh_sequence_len, mp_float_t refresh_time,
    const mcu_pin_obj_t *busy_pin, bool busy_state, mp_float_t seconds_per_frame,
    bool chip_select, bool grayscale, bool acep, bool two_byte_sequence_length, bool address_little_endian) {
    uint16_t color_depth = 1;
    bool core_grayscale = true;
    if (highlight_color != 0x000000) {
        self->core.colorspace.tricolor = true;
        self->core.colorspace.tricolor_hue = displayio_colorconverter_compute_hue(highlight_color);
        self->core.colorspace.tricolor_luma = displayio_colorconverter_compute_luma(highlight_color);
    } else {
        self->core.colorspace.tricolor = false;
    }
    self->acep = acep;
    self->core.colorspace.sevencolor = acep;
    if (acep) {
        color_depth = 4; // bits. 7 colors + clean
        grayscale = false;
        core_grayscale = false;
    }

    displayio_display_core_construct(&self->core, width, height, rotation, color_depth, core_grayscale, true, 1, true, true);
    displayio_display_bus_construct(&self->bus, bus, ram_width, ram_height,
        colstart, rowstart,
        set_column_window_command, set_row_window_command, set_current_column_command, set_current_row_command,
        false /* data_as_commands */, chip_select,
        false /* SH1107_addressing */, address_little_endian);

    self->write_black_ram_command = write_black_ram_command;
    self->black_bits_inverted = black_bits_inverted;
    self->write_color_ram_command = write_color_ram_command;
    self->color_bits_inverted = color_bits_inverted;
    self->refresh_time = refresh_time * 1000;
    self->busy_state = busy_state;
    self->refreshing = false;
    self->milliseconds_per_frame = seconds_per_frame * 1000;
    self->chip_select = chip_select ? CHIP_SELECT_TOGGLE_EVERY_BYTE : CHIP_SELECT_UNTOUCHED;
    self->grayscale = grayscale;

    self->start_sequence = start_sequence;
    self->start_sequence_len = start_sequence_len;
    self->start_up_time_ms = start_up_time * 1000;
    self->stop_sequence = stop_sequence;
    self->stop_sequence_len = stop_sequence_len;
    self->refresh_sequence = refresh_sequence;
    self->refresh_sequence_len = refresh_sequence_len;

    self->busy.base.type = &mp_type_NoneType;
    self->two_byte_sequence_length = two_byte_sequence_length;
    if (busy_pin != NULL) {
        self->busy.base.type = &digitalio_digitalinout_type;
        common_hal_digitalio_digitalinout_construct(&self->busy, busy_pin);
        common_hal_never_reset_pin(busy_pin);
    }

    // Clear the color memory if it isn't in use.
    if (highlight_color == 0x00 && write_color_ram_command != NO_COMMAND) {
        // TODO: Clear
    }

    // Set the group after initialization otherwise we may send pixels while we delay in
    // initialization.
    common_hal_epaperdisplay_epaperdisplay_set_root_group(self, &circuitpython_splash);
}

bool common_hal_epaperdisplay_epaperdisplay_set_root_group(epaperdisplay_epaperdisplay_obj_t *self, displayio_group_t *root_group) {
    return displayio_display_core_set_root_group(&self->core, root_group);
}

static const displayio_area_t *epaperdisplay_epaperdisplay_get_refresh_areas(epaperdisplay_epaperdisplay_obj_t *self) {
    if (self->core.full_refresh) {
        self->core.area.next = NULL;
        return &self->core.area;
    }
    const displayio_area_t *first_area = NULL;
    if (self->core.current_group != NULL) {
        first_area = displayio_group_get_refresh_areas(self->core.current_group, NULL);
    }
    if (first_area != NULL && self->bus.row_command == NO_COMMAND) {
        // Do a full refresh if the display doesn't support partial updates.
        self->core.area.next = NULL;
        return &self->core.area;
    }
    return first_area;
}

uint16_t common_hal_epaperdisplay_epaperdisplay_get_width(epaperdisplay_epaperdisplay_obj_t *self) {
    return displayio_display_core_get_width(&self->core);
}

uint16_t common_hal_epaperdisplay_epaperdisplay_get_height(epaperdisplay_epaperdisplay_obj_t *self) {
    return displayio_display_core_get_height(&self->core);
}

static void wait_for_busy(epaperdisplay_epaperdisplay_obj_t *self) {
    if (self->busy.base.type == &mp_type_NoneType) {
        return;
    }
    while (common_hal_digitalio_digitalinout_get_value(&self->busy) == self->busy_state &&
           !mp_hal_is_interrupted()) {
        RUN_BACKGROUND_TASKS;
    }
}

static void send_command_sequence(epaperdisplay_epaperdisplay_obj_t *self,
    bool should_wait_for_busy, const uint8_t *sequence, uint32_t sequence_len) {
    uint32_t i = 0;
    while (i < sequence_len) {
        const uint8_t *cmd = sequence + i;
        uint8_t data_size = *(cmd + 1);
        bool delay = (data_size & DELAY) != 0;
        const uint8_t *data = cmd + 2;
        data_size &= ~DELAY;
        if (self->two_byte_sequence_length) {
            data_size = ((data_size & ~DELAY) << 8) + *(cmd + 2);
            data = cmd + 3;
        }
        displayio_display_bus_begin_transaction(&self->bus);
        self->bus.send(self->bus.bus, DISPLAY_COMMAND, self->chip_select, cmd, 1);
        self->bus.send(self->bus.bus, DISPLAY_DATA, self->chip_select, data, data_size);
        displayio_display_bus_end_transaction(&self->bus);
        uint16_t delay_length_ms = 0;
        if (delay) {
            data_size++;
            delay_length_ms = *(cmd + 1 + data_size);
            if (delay_length_ms == 255) {
                delay_length_ms = 500;
            }
        }
        common_hal_time_delay_ms(delay_length_ms);
        if (should_wait_for_busy) {
            wait_for_busy(self);
        }
        if (mp_hal_is_interrupted()) {
            return;
        }
        i += 2 + data_size;
        if (self->two_byte_sequence_length) {
            i++;
        }
    }
}

void epaperdisplay_epaperdisplay_change_refresh_mode_parameters(epaperdisplay_epaperdisplay_obj_t *self,
    mp_buffer_info_t *start_sequence, float seconds_per_frame) {
    self->start_sequence = (uint8_t *)start_sequence->buf;
    self->start_sequence_len = start_sequence->len;
    self->milliseconds_per_frame = seconds_per_frame * 1000;
}

static void epaperdisplay_epaperdisplay_start_refresh(epaperdisplay_epaperdisplay_obj_t *self) {
    if (!displayio_display_bus_is_free(&self->bus)) {
        // Can't acquire display bus; skip updating this display. Try next display.
        return;
    }

    // run start sequence
    self->bus.bus_reset(self->bus.bus);

    common_hal_time_delay_ms(self->start_up_time_ms);

    send_command_sequence(self, true, self->start_sequence, self->start_sequence_len);
    if (mp_hal_is_interrupted()) {
        return;
    }
    displayio_display_core_start_refresh(&self->core);
}

uint32_t common_hal_epaperdisplay_epaperdisplay_get_time_to_refresh(epaperdisplay_epaperdisplay_obj_t *self) {
    if (self->core.last_refresh == 0) {
        return 0;
    }
    // Refresh at seconds per frame rate.
    uint32_t elapsed_time = supervisor_ticks_ms64() - self->core.last_refresh;
    if (elapsed_time > self->milliseconds_per_frame) {
        return 0;
    }
    return self->milliseconds_per_frame - elapsed_time;
}

static void epaperdisplay_epaperdisplay_finish_refresh(epaperdisplay_epaperdisplay_obj_t *self) {
    // Actually refresh the display now that all pixel RAM has been updated.
    send_command_sequence(self, false, self->refresh_sequence, self->refresh_sequence_len);

    supervisor_enable_tick();
    self->refreshing = true;

    displayio_display_core_finish_refresh(&self->core);
}

mp_obj_t common_hal_epaperdisplay_epaperdisplay_get_bus(epaperdisplay_epaperdisplay_obj_t *self) {
    return self->bus.bus;
}

void common_hal_epaperdisplay_epaperdisplay_set_rotation(epaperdisplay_epaperdisplay_obj_t *self, int rotation) {
    bool transposed = (self->core.rotation == 90 || self->core.rotation == 270);
    bool will_transposed = (rotation == 90 || rotation == 270);
    if (transposed != will_transposed) {
        int tmp = self->core.width;
        self->core.width = self->core.height;
        self->core.height = tmp;
    }
    displayio_display_core_set_rotation(&self->core, rotation);
    if (self == &displays[0].epaper_display) {
        supervisor_stop_terminal();
        supervisor_start_terminal(self->core.width, self->core.height);
    }
    if (self->core.current_group != NULL) {
        displayio_group_update_transform(self->core.current_group, &self->core.transform);
    }
}

uint16_t common_hal_epaperdisplay_epaperdisplay_get_rotation(epaperdisplay_epaperdisplay_obj_t *self) {
    return self->core.rotation;
}

mp_obj_t common_hal_epaperdisplay_epaperdisplay_get_root_group(epaperdisplay_epaperdisplay_obj_t *self) {
    if (self->core.current_group == NULL) {
        return mp_const_none;
    }
    return self->core.current_group;
}

static bool epaperdisplay_epaperdisplay_refresh_area(epaperdisplay_epaperdisplay_obj_t *self, const displayio_area_t *area) {
    uint16_t buffer_size = 128; // In uint32_ts

    displayio_area_t clipped;
    // Clip the area to the display by overlapping the areas. If there is no overlap then we're done.
    if (!displayio_display_core_clip_area(&self->core, area, &clipped)) {
        return true;
    }
    uint16_t subrectangles = 1;
    uint16_t rows_per_buffer = displayio_area_height(&clipped);
    uint8_t pixels_per_word = (sizeof(uint32_t) * 8) / self->core.colorspace.depth;
    uint16_t pixels_per_buffer = displayio_area_size(&clipped);
    if (displayio_area_size(&clipped) > buffer_size * pixels_per_word) {
        rows_per_buffer = buffer_size * pixels_per_word / displayio_area_width(&clipped);
        if (rows_per_buffer == 0) {
            rows_per_buffer = 1;
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
    volatile uint32_t mask_length = (pixels_per_buffer / 32) + 1;
    uint32_t mask[mask_length];

    uint8_t passes = 1;
    if (self->write_color_ram_command != NO_COMMAND) {
        passes = 2;
    }
    for (uint8_t pass = 0; pass < passes; pass++) {
        uint16_t remaining_rows = displayio_area_height(&clipped);

        if (self->bus.row_command != NO_COMMAND) {
            displayio_display_bus_set_region_to_update(&self->bus, &self->core, &clipped);
        }

        uint8_t write_command = self->write_black_ram_command;
        if (pass == 1) {
            write_command = self->write_color_ram_command;
        }
        displayio_display_bus_begin_transaction(&self->bus);
        self->bus.send(self->bus.bus, DISPLAY_COMMAND, self->chip_select, &write_command, 1);
        displayio_display_bus_end_transaction(&self->bus);

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


            uint16_t subrectangle_size_bytes = displayio_area_size(&subrectangle) / (8 / self->core.colorspace.depth);

            memset(mask, 0, mask_length * sizeof(mask[0]));
            memset(buffer, 0, buffer_size * sizeof(buffer[0]));

            if (!self->acep) {
                self->core.colorspace.grayscale = true;
                self->core.colorspace.grayscale_bit = 7;
            }
            if (pass == 1) {
                if (self->grayscale) { // 4-color grayscale
                    self->core.colorspace.grayscale_bit = 6;
                    displayio_display_core_fill_area(&self->core, &subrectangle, mask, buffer);
                } else if (self->core.colorspace.tricolor) {
                    self->core.colorspace.grayscale = false;
                    displayio_display_core_fill_area(&self->core, &subrectangle, mask, buffer);
                } else if (self->core.colorspace.sevencolor) {
                    displayio_display_core_fill_area(&self->core, &subrectangle, mask, buffer);
                }
            } else {
                displayio_display_core_fill_area(&self->core, &subrectangle, mask, buffer);
            }

            // Invert it all.
            if ((pass == 1 && self->color_bits_inverted) ||
                (pass == 0 && self->black_bits_inverted)) {
                for (uint16_t k = 0; k < buffer_size; k++) {
                    buffer[k] = ~buffer[k];
                }
            }

            if (!displayio_display_bus_begin_transaction(&self->bus)) {
                // Can't acquire display bus; skip the rest of the data. Try next display.
                return false;
            }
            self->bus.send(self->bus.bus, DISPLAY_DATA, self->chip_select, (uint8_t *)buffer, subrectangle_size_bytes);
            displayio_display_bus_end_transaction(&self->bus);

            // TODO(tannewt): Make refresh displays faster so we don't starve other
            // background tasks.
            #if CIRCUITPY_TINYUSB
            usb_background();
            #endif
        }
    }

    return true;
}

static bool _clean_area(epaperdisplay_epaperdisplay_obj_t *self) {
    uint16_t width = displayio_display_core_get_width(&self->core);
    uint16_t height = displayio_display_core_get_height(&self->core);

    uint8_t buffer[width / 2];
    memset(buffer, 0x77, width / 2);

    uint8_t write_command = self->write_black_ram_command;
    displayio_display_bus_begin_transaction(&self->bus);
    self->bus.send(self->bus.bus, DISPLAY_COMMAND, self->chip_select, &write_command, 1);
    displayio_display_bus_end_transaction(&self->bus);

    for (uint16_t j = 0; j < height; j++) {
        if (!displayio_display_bus_begin_transaction(&self->bus)) {
            // Can't acquire display bus; skip the rest of the data. Try next display.
            return false;
        }
        self->bus.send(self->bus.bus, DISPLAY_DATA, self->chip_select, buffer, width / 2);
        displayio_display_bus_end_transaction(&self->bus);

        // TODO(tannewt): Make refresh displays faster so we don't starve other
        // background tasks.
        #if CIRCUITPY_TINYUSB
        usb_background();
        #endif
    }

    return true;
}

bool common_hal_epaperdisplay_epaperdisplay_refresh(epaperdisplay_epaperdisplay_obj_t *self) {

    if (self->refreshing && self->busy.base.type == &digitalio_digitalinout_type) {
        if (common_hal_digitalio_digitalinout_get_value(&self->busy) != self->busy_state) {
            supervisor_disable_tick();
            self->refreshing = false;
            // Run stop sequence but don't wait for busy because busy is set when sleeping.
            send_command_sequence(self, false, self->stop_sequence, self->stop_sequence_len);
        } else {
            return false;
        }
    }
    if (self->core.current_group == NULL) {
        return true;
    }
    // Refresh at seconds per frame rate.
    if (common_hal_epaperdisplay_epaperdisplay_get_time_to_refresh(self) > 0) {
        return false;
    }
    if (!displayio_display_bus_is_free(&self->bus)) {
        // Can't acquire display bus; skip updating this display. Try next display.
        return false;
    }
    const displayio_area_t *current_area = epaperdisplay_epaperdisplay_get_refresh_areas(self);
    if (current_area == NULL) {
        return true;
    }
    if (self->acep) {
        epaperdisplay_epaperdisplay_start_refresh(self);
        _clean_area(self);
        epaperdisplay_epaperdisplay_finish_refresh(self);
        while (self->refreshing && !mp_hal_is_interrupted()) {
            RUN_BACKGROUND_TASKS;
        }
    }
    if (mp_hal_is_interrupted()) {
        return false;
    }

    epaperdisplay_epaperdisplay_start_refresh(self);
    while (current_area != NULL) {
        epaperdisplay_epaperdisplay_refresh_area(self, current_area);
        current_area = current_area->next;
    }
    epaperdisplay_epaperdisplay_finish_refresh(self);
    return true;
}

void epaperdisplay_epaperdisplay_background(epaperdisplay_epaperdisplay_obj_t *self) {
    if (self->refreshing) {
        bool refresh_done = false;
        if (self->busy.base.type == &digitalio_digitalinout_type) {
            bool busy = common_hal_digitalio_digitalinout_get_value(&self->busy);
            refresh_done = busy != self->busy_state;
        } else {
            refresh_done = supervisor_ticks_ms64() - self->core.last_refresh > self->refresh_time;
        }
        if (refresh_done) {
            supervisor_disable_tick();
            self->refreshing = false;
            // Run stop sequence but don't wait for busy because busy is set when sleeping.
            send_command_sequence(self, false, self->stop_sequence, self->stop_sequence_len);
        }
    }
}

bool common_hal_epaperdisplay_epaperdisplay_get_busy(epaperdisplay_epaperdisplay_obj_t *self) {
    epaperdisplay_epaperdisplay_background(self);
    return self->refreshing;
}

void release_epaperdisplay(epaperdisplay_epaperdisplay_obj_t *self) {
    if (self->refreshing) {
        wait_for_busy(self);
        supervisor_disable_tick();
        self->refreshing = false;
        // Run stop sequence but don't wait for busy because busy is set when sleeping.
        send_command_sequence(self, false, self->stop_sequence, self->stop_sequence_len);
    }

    release_display_core(&self->core);
    if (self->busy.base.type == &digitalio_digitalinout_type) {
        common_hal_digitalio_digitalinout_deinit(&self->busy);
    }
}

void epaperdisplay_epaperdisplay_reset(epaperdisplay_epaperdisplay_obj_t *self) {
    displayio_display_core_set_root_group(&self->core, &circuitpython_splash);
    self->core.full_refresh = true;
}

void epaperdisplay_epaperdisplay_collect_ptrs(epaperdisplay_epaperdisplay_obj_t *self) {
    displayio_display_core_collect_ptrs(&self->core);
    displayio_display_bus_collect_ptrs(&self->bus);
    gc_collect_ptr((void *)self->start_sequence);
    gc_collect_ptr((void *)self->stop_sequence);
    gc_collect_ptr((void *)self->refresh_sequence);
}

size_t maybe_refresh_epaperdisplay(void) {
    for (uint8_t i = 0; i < CIRCUITPY_DISPLAY_LIMIT; i++) {
        if (displays[i].epaper_display.base.type != &epaperdisplay_epaperdisplay_type ||
            displays[i].epaper_display.core.current_group != &circuitpython_splash) {
            // Skip regular displays and those not showing the splash.
            continue;
        }
        epaperdisplay_epaperdisplay_obj_t *display = &displays[i].epaper_display;
        size_t time_to_refresh = common_hal_epaperdisplay_epaperdisplay_get_time_to_refresh(display);
        if (time_to_refresh > 0) {
            return time_to_refresh;
        }
        if (common_hal_epaperdisplay_epaperdisplay_refresh(display)) {
            return 0;
        }
        // If we could refresh but it failed, then we want to retry.
        return 1;
    }
    // Return 0 if no ePaper displays are available to pretend it was updated.
    return 0;
}
