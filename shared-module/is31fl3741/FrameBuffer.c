// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 Mark Komus
//
// SPDX-License-Identifier: MIT

#include <string.h>

#include "py/gc.h"
#include "py/obj.h"
#include "py/objarray.h"
#include "py/objproperty.h"
#include "py/runtime.h"

#include "shared-bindings/is31fl3741/IS31FL3741.h"
#include "shared-bindings/is31fl3741/FrameBuffer.h"
#include "shared-bindings/util.h"
#include "shared-module/framebufferio/FramebufferDisplay.h"
#include "shared-bindings/busio/I2C.h"
#include "supervisor/port.h"

void common_hal_is31fl3741_framebuffer_construct(is31fl3741_framebuffer_obj_t *self, int width, int height, mp_obj_t framebuffer, is31fl3741_IS31FL3741_obj_t *is31, mp_obj_t mapping) {
    self->width = width;
    self->height = height;

    self->bufsize = sizeof(uint32_t) * width * height;

    self->is31fl3741 = is31;

    common_hal_busio_i2c_never_reset(self->is31fl3741->i2c);

    mp_obj_t *items;
    size_t len;
    mp_obj_tuple_get(mapping, &len, &items);

    if (len != (size_t)(self->scale_width * self->scale_height * 3)) {
        mp_raise_ValueError(MP_ERROR_TEXT("LED mappings must match display size"));
    }

    self->mapping = port_malloc(sizeof(uint16_t) * len, false);
    if (self->mapping == NULL) {
        m_malloc_fail(sizeof(uint16_t) * len);
    }
    for (size_t i = 0; i < len; i++) {
        mp_int_t value = mp_obj_get_int(items[i]);
        // We only store up to 16 bits
        if (value > 0xFFFF) {
            value = 0xFFFF;
        }
        self->mapping[i] = (uint16_t)value;
    }

    common_hal_is31fl3741_framebuffer_reconstruct(self, framebuffer);
}

void common_hal_is31fl3741_framebuffer_reconstruct(is31fl3741_framebuffer_obj_t *self, mp_obj_t framebuffer) {
    self->paused = 1;

    if (framebuffer) {
        self->framebuffer = framebuffer;
        mp_get_buffer_raise(self->framebuffer, &self->bufinfo, MP_BUFFER_READ);
        if (mp_get_buffer(self->framebuffer, &self->bufinfo, MP_BUFFER_RW)) {
            self->bufinfo.typecode = 'H' | MP_OBJ_ARRAY_TYPECODE_FLAG_RW;
        } else {
            self->bufinfo.typecode = 'H';
        }

        // verify that the matrix is big enough
        mp_get_index(mp_obj_get_type(self->framebuffer), self->bufinfo.len, MP_OBJ_NEW_SMALL_INT(self->bufsize - 1), false);
    } else {
        if (self->framebuffer == NULL && self->bufinfo.buf != NULL) {
            port_free(self->bufinfo.buf);
        }

        self->framebuffer = NULL;
        self->bufinfo.buf = port_malloc(self->bufsize, false);
        if (self->bufinfo.buf == NULL) {
            return;
        }
        self->bufinfo.len = self->bufsize;
        self->bufinfo.typecode = 'H' | MP_OBJ_ARRAY_TYPECODE_FLAG_RW;
    }

    common_hal_is31fl3741_begin_transaction(self->is31fl3741);
    common_hal_is31fl3741_send_reset(self->is31fl3741);
    common_hal_is31fl3741_set_current(self->is31fl3741, 0xFE);

    // set scale (brightness) to max for all LEDs
    for (int i = 0; i < 351; i++) {
        common_hal_is31fl3741_set_led(self->is31fl3741, i, 0xFF, 2);
    }

    common_hal_is31fl3741_send_enable(self->is31fl3741);
    common_hal_is31fl3741_end_transaction(self->is31fl3741);

    self->paused = 0;
}

void common_hal_is31fl3741_framebuffer_deinit(is31fl3741_framebuffer_obj_t *self) {
    common_hal_is31fl3741_end_transaction(self->is31fl3741); // in case we still had a lock

    common_hal_is31fl3741_IS31FL3741_deinit(self->is31fl3741);

    if (self->mapping != NULL) {
        port_free(self->mapping);
        self->mapping = NULL;
    }

    if (self->framebuffer == NULL && self->bufinfo.buf != NULL) {
        port_free(self->bufinfo.buf);
    }

    self->base.type = NULL;

    // If a framebuffer was passed in to the constructor, NULL the reference
    // here so that it will become GC'able
    self->framebuffer = NULL;
}

void common_hal_is31fl3741_framebuffer_set_paused(is31fl3741_framebuffer_obj_t *self, bool paused) {
    self->paused = paused;
}

bool common_hal_is31fl3741_framebuffer_get_paused(is31fl3741_framebuffer_obj_t *self) {
    return self->paused;
}

void common_hal_is31fl3741_framebuffer_refresh(is31fl3741_framebuffer_obj_t *self, uint8_t *dirtyrows) {
    if (!self->paused) {
        common_hal_is31fl3741_begin_transaction(self->is31fl3741);

        uint8_t dirty_row_flags = 0xFF;
        if (self->scale) {
            // Based on the Arduino IS31FL3741 driver code
            // dirtyrows flag current not implemented for scaled displays
            uint32_t *buffer = self->bufinfo.buf;

            for (int x = 0; x < self->scale_width; x++) {
                uint32_t *ptr = &buffer[x * 3]; // Entry along top scan line w/x offset
                for (int y = 0; y < self->scale_height; y++) {
                    uint16_t rsum = 0, gsum = 0, bsum = 0;
                    // Inner x/y loops are row-major on purpose (less pointer math)
                    for (uint8_t yy = 0; yy < 3; yy++) {
                        for (uint8_t xx = 0; xx < 3; xx++) {
                            uint32_t rgb = ptr[xx];
                            rsum += rgb >> 16 & 0xFF;
                            gsum += (rgb >> 8) & 0xFF;
                            bsum += rgb & 0xFF;
                        }
                        ptr += self->width; // Advance one scan line
                    }
                    rsum = rsum / 9;
                    gsum = gsum / 9;
                    bsum = bsum / 9;
                    uint32_t color = 0;
                    if (self->auto_gamma) {
                        color = (IS31GammaTable[rsum] << 16) +
                            (IS31GammaTable[gsum] << 8) +
                            IS31GammaTable[bsum];
                    } else {
                        color = (rsum << 16) + (gsum << 8) + bsum;
                    }
                    common_hal_is31fl3741_draw_pixel(self->is31fl3741, x, y, color, self->mapping, self->scale_height);
                }
            }
        } else {
            uint32_t *buffer = self->bufinfo.buf;
            for (int y = 0; y < self->height; y++) {
                if ((dirtyrows != 0) && ((y % 8) == 0)) {
                    dirty_row_flags = *dirtyrows++;
                }

                if ((dirty_row_flags >> (y % 8)) & 0x1) {
                    for (int x = 0; x < self->width; x++) {
                        uint32_t color = 0;
                        if (self->auto_gamma) {
                            color = (IS31GammaTable[((*buffer) >> 16 & 0xFF)] << 16) +
                                (IS31GammaTable[((*buffer) >> 8 & 0xFF)] << 8) +
                                IS31GammaTable[((*buffer) & 0xFF)];
                        } else {
                            color = *buffer;
                        }

                        common_hal_is31fl3741_draw_pixel(self->is31fl3741, x, y, color, self->mapping, self->scale_height);
                        buffer++;
                    }
                } else {
                    buffer += self->width; // row did not have to be redrawn, skip it in the buffer
                }
            }
        }
        common_hal_is31fl3741_end_transaction(self->is31fl3741);
    }
}

int common_hal_is31fl3741_framebuffer_get_width(is31fl3741_framebuffer_obj_t *self) {
    return self->width;
}

int common_hal_is31fl3741_framebuffer_get_height(is31fl3741_framebuffer_obj_t *self) {
    return self->height;
}

void is31fl3741_framebuffer_collect_ptrs(is31fl3741_framebuffer_obj_t *self) {
    gc_collect_ptr(self->framebuffer);
    gc_collect_ptr(self->is31fl3741->i2c);
    gc_collect_ptr(self->is31fl3741);
}
