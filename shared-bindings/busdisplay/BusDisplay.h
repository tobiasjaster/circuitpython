// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017, 2018 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#include "common-hal/microcontroller/Pin.h"

#include "shared-module/busdisplay/BusDisplay.h"
#include "shared-module/displayio/Group.h"

extern const mp_obj_type_t busdisplay_busdisplay_type;

#define NO_BRIGHTNESS_COMMAND 0x100
#define NO_FPS_LIMIT 0xffffffff

void common_hal_busdisplay_busdisplay_construct(busdisplay_busdisplay_obj_t *self,
    mp_obj_t bus, uint16_t width, uint16_t height,
    int16_t colstart, int16_t rowstart, uint16_t rotation, uint16_t color_depth, bool grayscale,
    bool pixels_in_byte_share_row, uint8_t bytes_per_cell, bool reverse_pixels_in_byte, bool reverse_bytes_in_word,
    uint8_t set_column_command, uint8_t set_row_command, uint8_t write_ram_command,
    uint8_t *init_sequence, uint16_t init_sequence_len, const mcu_pin_obj_t *backlight_pin, uint16_t brightness_command,
    mp_float_t brightness,
    bool single_byte_bounds, bool data_as_commands, bool auto_refresh, uint16_t native_frames_per_second,
    bool backlight_on_high, bool SH1107_addressing, uint16_t backlight_pwm_frequency);

bool common_hal_busdisplay_busdisplay_refresh(busdisplay_busdisplay_obj_t *self, uint32_t target_ms_per_frame, uint32_t maximum_ms_per_real_frame);

bool common_hal_busdisplay_busdisplay_get_auto_refresh(busdisplay_busdisplay_obj_t *self);
void common_hal_busdisplay_busdisplay_set_auto_refresh(busdisplay_busdisplay_obj_t *self, bool auto_refresh);

uint16_t common_hal_busdisplay_busdisplay_get_width(busdisplay_busdisplay_obj_t *self);
uint16_t common_hal_busdisplay_busdisplay_get_height(busdisplay_busdisplay_obj_t *self);
uint16_t common_hal_busdisplay_busdisplay_get_rotation(busdisplay_busdisplay_obj_t *self);
void common_hal_busdisplay_busdisplay_set_rotation(busdisplay_busdisplay_obj_t *self, int rotation);

bool common_hal_busdisplay_busdisplay_get_dither(busdisplay_busdisplay_obj_t *self);
void common_hal_busdisplay_busdisplay_set_dither(busdisplay_busdisplay_obj_t *self, bool dither);

mp_float_t common_hal_busdisplay_busdisplay_get_brightness(busdisplay_busdisplay_obj_t *self);
bool common_hal_busdisplay_busdisplay_set_brightness(busdisplay_busdisplay_obj_t *self, mp_float_t brightness);

mp_obj_t common_hal_busdisplay_busdisplay_get_bus(busdisplay_busdisplay_obj_t *self);
mp_obj_t common_hal_busdisplay_busdisplay_get_root_group(busdisplay_busdisplay_obj_t *self);
mp_obj_t common_hal_busdisplay_busdisplay_set_root_group(busdisplay_busdisplay_obj_t *self, displayio_group_t *root_group);
