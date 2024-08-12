// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2018 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/displayio/ColorConverter.h"

#include "py/misc.h"
#include "py/runtime.h"

#define NO_TRANSPARENT_COLOR (0x1000000)

uint32_t displayio_colorconverter_dither_noise_1(uint32_t n) {
    n = (n >> 13) ^ n;
    int nn = (n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff;
    return (uint32_t)(((float)nn / (1073741824.0f * 2)) * 255);
}

uint32_t displayio_colorconverter_dither_noise_2(uint32_t x, uint32_t y) {
    return displayio_colorconverter_dither_noise_1(x + y * 0xFFFF);
}

void common_hal_displayio_colorconverter_construct(displayio_colorconverter_t *self, bool dither, displayio_colorspace_t input_colorspace) {
    self->dither = dither;
    self->transparent_color = NO_TRANSPARENT_COLOR;
    self->input_colorspace = input_colorspace;
    self->output_colorspace.depth = 16;
}

uint16_t displayio_colorconverter_compute_rgb565(uint32_t color_rgb888) {
    uint32_t r5 = (color_rgb888 >> 19);
    uint32_t g6 = (color_rgb888 >> 10) & 0x3f;
    uint32_t b5 = (color_rgb888 >> 3) & 0x1f;
    return r5 << 11 | g6 << 5 | b5;
}

uint8_t displayio_colorconverter_compute_rgb332(uint32_t color_rgb888) {
    uint32_t r3 = (color_rgb888 >> 21);
    uint32_t g3 = (color_rgb888 >> 13) & 0x7;
    uint32_t b2 = (color_rgb888 >> 6) & 0x3;
    return r3 << 5 | g3 << 2 | b2;
}

uint8_t displayio_colorconverter_compute_rgbd(uint32_t color_rgb888) {
    uint32_t r1 = (color_rgb888 >> 23) & 0x1;
    uint32_t g1 = (color_rgb888 >> 15) & 0x1;
    uint32_t b1 = (color_rgb888 >> 7) & 0x1;
    return r1 << 3 | g1 << 2 | b1 << 1 /* | dummy */;
}

uint8_t displayio_colorconverter_compute_luma(uint32_t color_rgb888) {
    uint32_t r8 = (color_rgb888 >> 16);
    uint32_t g8 = (color_rgb888 >> 8) & 0xff;
    uint32_t b8 = color_rgb888 & 0xff;
    return (r8 * 19 + g8 * 182 + b8 * 54) / 255;
}

uint8_t displayio_colorconverter_compute_chroma(uint32_t color_rgb888) {
    uint32_t r8 = (color_rgb888 >> 16);
    uint32_t g8 = (color_rgb888 >> 8) & 0xff;
    uint32_t b8 = color_rgb888 & 0xff;
    uint8_t max = MAX(r8, MAX(g8, b8));
    uint8_t min = MIN(r8, MIN(g8, b8));
    return max - min;
}

uint8_t displayio_colorconverter_compute_hue(uint32_t color_rgb888) {
    uint32_t r8 = (color_rgb888 >> 16);
    uint32_t g8 = (color_rgb888 >> 8) & 0xff;
    uint32_t b8 = color_rgb888 & 0xff;
    uint8_t max = MAX(r8, MAX(g8, b8));
    uint8_t min = MIN(r8, MIN(g8, b8));
    uint8_t c = max - min;
    if (c == 0) {
        return 0;
    }

    int32_t hue = 0;
    if (max == r8) {
        hue = (((int32_t)(g8 - b8) * 40) / c) % 240;
    } else if (max == g8) {
        hue = (((int32_t)(b8 - r8) + (2 * c)) * 40) / c;
    } else if (max == b8) {
        hue = (((int32_t)(r8 - g8) + (4 * c)) * 40) / c;
    }
    if (hue < 0) {
        hue += 240;
    }

    return hue;
}

uint8_t displayio_colorconverter_compute_sevencolor(uint32_t color_rgb888) {
    // This is DDX=1, the default for the displays.
    uint8_t chroma = displayio_colorconverter_compute_chroma(color_rgb888);
    if (chroma >= 64) {
        uint8_t hue = displayio_colorconverter_compute_hue(color_rgb888);
        // Red 0
        if (hue < 10) {
            return 0x4;
        }
        // Orange 21
        if (hue < 21 + 10) {
            return 0x6;
        }
        // Yellow 42
        if (hue < 42 + 21) {
            return 0x5;
        }
        // Green 85
        if (hue < 85 + 42) {
            return 0x2;
        }
        // Blue 170
        if (hue < 170 + 42) {
            return 0x3;
        }

        // The rest is red to 255
        return 0x4;
    } else {
        uint8_t luma = displayio_colorconverter_compute_luma(color_rgb888);
        if (luma >= 128) {
            return 0x1; // White
        } else {
            return 0x0; // Black
        }
    }
}

void displayio_colorconverter_compute_tricolor(const _displayio_colorspace_t *colorspace, uint8_t pixel_hue, uint32_t *color) {

    int16_t hue_diff = colorspace->tricolor_hue - pixel_hue;
    if ((-10 <= hue_diff && hue_diff <= 10) || hue_diff <= -220 || hue_diff >= 220) {
        if (colorspace->grayscale) {
            *color = 0;
        } else {
            *color = 1;
        }
    } else if (!colorspace->grayscale) {
        *color = 0;
    }
}

void common_hal_displayio_colorconverter_convert(displayio_colorconverter_t *self, const _displayio_colorspace_t *colorspace, uint32_t input_color, uint32_t *output_color) {
    displayio_input_pixel_t input_pixel;
    input_pixel.pixel = input_color;
    input_pixel.x = input_pixel.y = input_pixel.tile = input_pixel.tile_x = input_pixel.tile_y = 0;

    displayio_output_pixel_t output_pixel;
    output_pixel.pixel = 0;
    output_pixel.opaque = false;

    displayio_colorconverter_convert(self, colorspace, &input_pixel, &output_pixel);

    (*output_color) = output_pixel.pixel;
}

void common_hal_displayio_colorconverter_set_dither(displayio_colorconverter_t *self, bool dither) {
    self->dither = dither;
}

bool common_hal_displayio_colorconverter_get_dither(displayio_colorconverter_t *self) {
    return self->dither;
}

void common_hal_displayio_colorconverter_make_transparent(displayio_colorconverter_t *self, uint32_t transparent_color) {
    if (self->transparent_color != NO_TRANSPARENT_COLOR) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Only one color can be transparent at a time"));
    }
    self->transparent_color = transparent_color;
}

void common_hal_displayio_colorconverter_make_opaque(displayio_colorconverter_t *self, uint32_t transparent_color) {
    (void)transparent_color;
    // NO_TRANSPARENT_COLOR will never equal a valid color
    self->transparent_color = NO_TRANSPARENT_COLOR;
}


// Convert a single input pixel to RGB888
uint32_t displayio_colorconverter_convert_pixel(displayio_colorspace_t colorspace, uint32_t pixel) {
    switch (colorspace) {
        case DISPLAYIO_COLORSPACE_RGB565_SWAPPED:
            pixel = __builtin_bswap16(pixel);
            MP_FALLTHROUGH;
        case DISPLAYIO_COLORSPACE_RGB565: {
            uint32_t r8 = (pixel >> 11) << 3;
            uint32_t g8 = ((pixel >> 5) << 2) & 0xff;
            uint32_t b8 = (pixel << 3) & 0xff;
            pixel = (r8 << 16) | (g8 << 8) | b8;
        }
        break;

        case DISPLAYIO_COLORSPACE_RGB555_SWAPPED:
            pixel = __builtin_bswap16(pixel);
            MP_FALLTHROUGH;
        case DISPLAYIO_COLORSPACE_RGB555: {
            uint32_t r8 = (pixel >> 10) << 3;
            uint32_t g8 = ((pixel >> 5) << 3) & 0xff;
            uint32_t b8 = (pixel << 3) & 0xff;
            pixel = (r8 << 16) | (g8 << 8) | b8;
        }
        break;

        case DISPLAYIO_COLORSPACE_BGR565_SWAPPED:
            pixel = __builtin_bswap16(pixel);
            MP_FALLTHROUGH;
        case DISPLAYIO_COLORSPACE_BGR565: {
            uint32_t b8 = (pixel >> 11) << 3;
            uint32_t g8 = ((pixel >> 5) << 2) & 0xff;
            uint32_t r8 = (pixel << 3) & 0xff;
            pixel = (r8 << 16) | (g8 << 8) | b8;
        }
        break;

        case DISPLAYIO_COLORSPACE_BGR555_SWAPPED:
            pixel = __builtin_bswap16(pixel);
            MP_FALLTHROUGH;
        case DISPLAYIO_COLORSPACE_BGR555: {
            uint32_t b8 = (pixel >> 10) << 3;
            uint32_t g8 = ((pixel >> 5) << 3) & 0xff;
            uint32_t r8 = (pixel << 3) & 0xff;
            pixel = (r8 << 16) | (g8 << 8) | b8;
        }
        break;

        default:
        case DISPLAYIO_COLORSPACE_RGB888:
            break;

        case DISPLAYIO_COLORSPACE_L8: {
            uint32_t l8 = pixel & 0xff;
            pixel = l8 * 0x010101;
        }
        break;
    }

    return pixel;
}

void displayio_convert_color(const _displayio_colorspace_t *colorspace, bool dither, const displayio_input_pixel_t *input_pixel, displayio_output_pixel_t *output_color) {
    uint32_t pixel = input_pixel->pixel;
    if (dither) {
        uint8_t randr = (displayio_colorconverter_dither_noise_2(input_pixel->tile_x, input_pixel->tile_y));
        uint8_t randg = (displayio_colorconverter_dither_noise_2(input_pixel->tile_x + 33, input_pixel->tile_y));
        uint8_t randb = (displayio_colorconverter_dither_noise_2(input_pixel->tile_x, input_pixel->tile_y + 33));

        uint32_t r8 = (pixel >> 16);
        uint32_t g8 = (pixel >> 8) & 0xff;
        uint32_t b8 = pixel & 0xff;

        if (colorspace->depth == 16) {
            b8 = MIN(255, b8 + (randb & 0x07));
            r8 = MIN(255, r8 + (randr & 0x07));
            g8 = MIN(255, g8 + (randg & 0x03));
        } else {
            int bitmask = 0xFF >> colorspace->depth;
            b8 = MIN(255, b8 + (randb & bitmask));
            r8 = MIN(255, r8 + (randr & bitmask));
            g8 = MIN(255, g8 + (randg & bitmask));
        }
        pixel = r8 << 16 | g8 << 8 | b8;
    }

    if (colorspace->depth == 16) {
        uint16_t packed = displayio_colorconverter_compute_rgb565(pixel);
        if (colorspace->reverse_bytes_in_word) {
            // swap bytes
            packed = __builtin_bswap16(packed);
        }
        output_color->pixel = packed;
        output_color->opaque = true;
        return;
    } else if (colorspace->tricolor) {
        uint8_t luma = displayio_colorconverter_compute_luma(pixel);
        output_color->pixel = luma >> (8 - colorspace->depth);
        if (displayio_colorconverter_compute_chroma(pixel) <= 16) {
            if (!colorspace->grayscale) {
                output_color->pixel = 0;
            }
            output_color->opaque = true;
            return;
        }
        uint8_t pixel_hue = displayio_colorconverter_compute_hue(pixel);
        displayio_colorconverter_compute_tricolor(colorspace, pixel_hue, &output_color->pixel);
        return;
    } else if (colorspace->grayscale && colorspace->depth <= 8) {
        uint8_t luma = displayio_colorconverter_compute_luma(pixel);
        size_t bitmask = (1 << colorspace->depth) - 1;
        output_color->pixel = (luma >> colorspace->grayscale_bit) & bitmask;
        output_color->opaque = true;
        return;
    } else if (colorspace->depth == 32) {
        output_color->pixel = pixel;
        output_color->opaque = true;
        return;
    } else if (colorspace->depth == 8 && !colorspace->grayscale) {
        uint8_t packed = displayio_colorconverter_compute_rgb332(pixel);
        output_color->pixel = packed;
        output_color->opaque = true;
        return;
    } else if (colorspace->depth == 4) {
        uint8_t packed;
        if (colorspace->sevencolor) {
            packed = displayio_colorconverter_compute_sevencolor(pixel);
        } else {
            packed = displayio_colorconverter_compute_rgbd(pixel);
        }
        output_color->pixel = packed;
        output_color->opaque = true;
        return;
    }
    output_color->opaque = false;
}

void displayio_colorconverter_convert(displayio_colorconverter_t *self, const _displayio_colorspace_t *colorspace, const displayio_input_pixel_t *input_pixel, displayio_output_pixel_t *output_color) {
    uint32_t pixel = input_pixel->pixel;

    if (self->transparent_color == pixel) {
        output_color->opaque = false;
        return;
    }

    if (!self->dither && self->cached_colorspace == colorspace && self->cached_input_pixel == input_pixel->pixel) {
        output_color->pixel = self->cached_output_color;
        return;
    }

    displayio_input_pixel_t rgb888_pixel = *input_pixel;
    rgb888_pixel.pixel = displayio_colorconverter_convert_pixel(self->input_colorspace, input_pixel->pixel);
    displayio_convert_color(colorspace, self->dither, &rgb888_pixel, output_color);

    if (!self->dither) {
        self->cached_colorspace = colorspace;
        self->cached_input_pixel = input_pixel->pixel;
        self->cached_output_color = output_color->pixel;
    }
}



// Currently no refresh logic is needed for a ColorConverter.
bool displayio_colorconverter_needs_refresh(displayio_colorconverter_t *self) {
    return false;
}

void displayio_colorconverter_finish_refresh(displayio_colorconverter_t *self) {
}
