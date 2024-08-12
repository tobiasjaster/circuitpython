// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2018 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/displayio/Bitmap.h"

#include <string.h>

#include "py/runtime.h"
#include "py/gc.h"

enum { ALIGN_BITS = 8 * sizeof(uint32_t) };

static int stride(uint32_t width, uint32_t bits_per_value) {
    uint32_t row_width = width * bits_per_value;
    // align to uint32_t
    return (row_width + ALIGN_BITS - 1) / ALIGN_BITS;
}

void common_hal_displayio_bitmap_construct(displayio_bitmap_t *self, uint32_t width,
    uint32_t height, uint32_t bits_per_value) {
    common_hal_displayio_bitmap_construct_from_buffer(self, width, height, bits_per_value, NULL, false);
}

void common_hal_displayio_bitmap_construct_from_buffer(displayio_bitmap_t *self, uint32_t width,
    uint32_t height, uint32_t bits_per_value, uint32_t *data, bool read_only) {
    self->width = width;
    self->height = height;
    self->stride = stride(width, bits_per_value);
    self->data_alloc = false;
    if (!data) {
        data = m_malloc(self->stride * height * sizeof(uint32_t));
        self->data_alloc = true;
    }
    self->data = data;
    self->read_only = read_only;
    self->bits_per_value = bits_per_value;

    if (bits_per_value > 8 && bits_per_value != 16 && bits_per_value != 32) {
        mp_raise_NotImplementedError(MP_ERROR_TEXT("Invalid bits per value"));
    }

    // Division and modulus can be slow because it has to handle any integer. We know bits_per_value
    // is a power of two. We divide and mod by bits_per_value to compute the offset into the byte
    // array. So, we can the offset computation to simplify to a shift for division and mask for mod.

    self->x_shift = 0; // Used to divide the index by the number of pixels per word. Its used in a
                       // shift which effectively divides by 2 ** x_shift.
    uint32_t power_of_two = 1;
    while (power_of_two < 8 / bits_per_value) {
        self->x_shift++;
        power_of_two <<= 1;
    }
    self->x_mask = (1u << self->x_shift) - 1u; // Used as a modulus on the x value
    self->bitmask = (1u << bits_per_value) - 1u;

    self->dirty_area.x1 = 0;
    self->dirty_area.x2 = width;
    self->dirty_area.y1 = 0;
    self->dirty_area.y2 = height;
}

void common_hal_displayio_bitmap_deinit(displayio_bitmap_t *self) {
    if (self->data_alloc) {
        gc_free(self->data);
    }
    self->data = NULL;
}

bool common_hal_displayio_bitmap_deinited(displayio_bitmap_t *self) {
    return self->data == NULL;
}

uint16_t common_hal_displayio_bitmap_get_height(displayio_bitmap_t *self) {
    return self->height;
}

uint16_t common_hal_displayio_bitmap_get_width(displayio_bitmap_t *self) {
    return self->width;
}

uint32_t common_hal_displayio_bitmap_get_bits_per_value(displayio_bitmap_t *self) {
    return self->bits_per_value;
}

uint32_t common_hal_displayio_bitmap_get_pixel(displayio_bitmap_t *self, int16_t x, int16_t y) {
    if (x >= self->width || x < 0 || y >= self->height || y < 0) {
        return 0;
    }
    int32_t row_start = y * self->stride;
    uint32_t *row = self->data + row_start;
    uint8_t bytes_per_value = self->bits_per_value / 8;
    uint8_t values_per_byte = 8 / self->bits_per_value;
    if (bytes_per_value < 1) {
        uint8_t bits = ((uint8_t *)row)[x >> self->x_shift];
        uint8_t bit_position = (values_per_byte - (x & self->x_mask) - 1) * self->bits_per_value;
        return (bits >> bit_position) & self->bitmask;
    } else {
        if (bytes_per_value == 1) {
            return ((uint8_t *)row)[x];
        } else if (bytes_per_value == 2) {
            return ((uint16_t *)row)[x];
        } else if (bytes_per_value == 4) {
            return ((uint32_t *)row)[x];
        }
    }
    return 0;
}

void displayio_bitmap_set_dirty_area(displayio_bitmap_t *self, const displayio_area_t *dirty_area) {
    if (self->read_only) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Read-only"));
    }

    displayio_area_t area = *dirty_area;
    displayio_area_canon(&area);
    displayio_area_union(&area, &self->dirty_area, &area);
    displayio_area_t bitmap_area = {0, 0, self->width, self->height, NULL};
    displayio_area_compute_overlap(&area, &bitmap_area, &self->dirty_area);
}

void displayio_bitmap_write_pixel(displayio_bitmap_t *self, int16_t x, int16_t y, uint32_t value) {
    if (self->read_only) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Read-only"));
    }
    // Writes the color index value into a pixel position
    // Must update the dirty area separately

    // Don't write if out of area
    if (0 > x || x >= self->width || 0 > y || y >= self->height) {
        return;
    }

    // Update one pixel of data
    int32_t row_start = y * self->stride;
    uint32_t *row = self->data + row_start;
    uint8_t bytes_per_value = self->bits_per_value / 8;
    uint8_t values_per_byte = 8 / self->bits_per_value;
    if (bytes_per_value < 1) {
        uint8_t bits = ((uint8_t *)row)[x >> self->x_shift];
        uint8_t bit_position = (values_per_byte - (x & self->x_mask) - 1) * self->bits_per_value;
        bits &= ~(self->bitmask << bit_position);
        bits |= (value & self->bitmask) << bit_position;
        ((uint8_t *)row)[x >> self->x_shift] = bits;
    } else {
        if (bytes_per_value == 1) {
            ((uint8_t *)row)[x] = value;
        } else if (bytes_per_value == 2) {
            ((uint16_t *)row)[x] = value;
        } else if (bytes_per_value == 4) {
            ((uint32_t *)row)[x] = value;
        }
    }
}

void common_hal_displayio_bitmap_set_pixel(displayio_bitmap_t *self, int16_t x, int16_t y, uint32_t value) {
    if (self->read_only) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Read-only"));
    }
    // update the dirty region
    displayio_area_t a = {x, y, x + 1, y + 1, NULL};
    displayio_bitmap_set_dirty_area(self, &a);

    // write the pixel
    displayio_bitmap_write_pixel(self, x, y, value);

}

displayio_area_t *displayio_bitmap_get_refresh_areas(displayio_bitmap_t *self, displayio_area_t *tail) {
    if (self->dirty_area.x1 == self->dirty_area.x2 || self->read_only) {
        return tail;
    }
    self->dirty_area.next = tail;
    return &self->dirty_area;
}

void displayio_bitmap_finish_refresh(displayio_bitmap_t *self) {
    if (self->read_only) {
        return;
    }
    self->dirty_area.x1 = 0;
    self->dirty_area.x2 = 0;
}

void common_hal_displayio_bitmap_fill(displayio_bitmap_t *self, uint32_t value) {
    if (self->read_only) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Read-only"));
    }
    displayio_area_t a = {0, 0, self->width, self->height, NULL};
    displayio_bitmap_set_dirty_area(self, &a);

    // build the packed word
    uint32_t word = 0;
    for (uint8_t i = 0; i < 32 / self->bits_per_value; i++) {
        word |= (value & self->bitmask) << (32 - ((i + 1) * self->bits_per_value));
    }
    // copy it in
    for (uint32_t i = 0; i < self->stride * self->height; i++) {
        self->data[i] = word;
    }
}

int common_hal_displayio_bitmap_get_buffer(displayio_bitmap_t *self, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    if ((flags & MP_BUFFER_WRITE) && self->read_only) {
        return 1;
    }
    bufinfo->len = self->stride * self->height * sizeof(uint32_t);
    bufinfo->buf = self->data;
    switch (self->bits_per_value) {
        case 32:
            bufinfo->typecode = 'I';
            break;
        case 16:
            bufinfo->typecode = 'H';
            break;
        default:
            bufinfo->typecode = 'B';
            break;
    }
    return 0;
}
