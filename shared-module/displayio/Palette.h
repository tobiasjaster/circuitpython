// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2018 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "py/obj.h"

typedef struct {
    uint8_t depth;
    uint8_t bytes_per_cell;
    uint8_t tricolor_hue;
    uint8_t tricolor_luma;
    uint8_t grayscale_bit; // The lowest grayscale bit. Normally 8 - depth.
    bool grayscale;
    bool tricolor;
    bool sevencolor; // Acep e-ink screens.
    bool pixels_in_byte_share_row;
    bool reverse_pixels_in_byte;
    bool reverse_bytes_in_word;
    bool dither;
} _displayio_colorspace_t;

typedef struct {
    uint32_t rgb888;
    const _displayio_colorspace_t *cached_colorspace;
    uint32_t cached_color;
    uint8_t cached_colorspace_grayscale_bit;
    bool cached_colorspace_grayscale;
    bool transparent; // This may have additional bits added later for blending.
} _displayio_color_t;

typedef struct {
    uint32_t pixel;
    uint16_t x;
    uint16_t y;
    uint8_t tile;
    uint16_t tile_x;
    uint16_t tile_y;
} displayio_input_pixel_t;

typedef struct {
    uint32_t pixel;
    bool opaque;
} displayio_output_pixel_t;

typedef struct displayio_palette {
    mp_obj_base_t base;
    _displayio_color_t *colors;
    uint32_t color_count;
    bool needs_refresh;
    bool dither;
} displayio_palette_t;


void displayio_palette_get_color(displayio_palette_t *palette, const _displayio_colorspace_t *colorspace, const displayio_input_pixel_t *input_pixel, displayio_output_pixel_t *output_color);
;
bool displayio_palette_needs_refresh(displayio_palette_t *self);
void displayio_palette_finish_refresh(displayio_palette_t *self);
