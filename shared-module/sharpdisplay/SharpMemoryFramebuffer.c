// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Jeff Epler for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <string.h>

#include "py/gc.h"

#include "shared-bindings/board/__init__.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/sharpdisplay/SharpMemoryFramebuffer.h"
#include "shared-module/sharpdisplay/SharpMemoryFramebuffer.h"
#include "supervisor/port.h"

#define SHARPMEM_BIT_WRITECMD_LSB (0x80)
#define JDI_BIT_WRITECMD_LSB      (0x90)
#define SHARPMEM_BIT_VCOM_LSB (0x40)

static uint8_t bitrev(uint8_t n) {
    uint8_t r = 0;
    for (int i = 0; i < 8; i++) {r |= ((n >> i) & 1) << (7 - i);
    }
    return r;
}

int common_hal_sharpdisplay_framebuffer_get_width(sharpdisplay_framebuffer_obj_t *self) {
    return self->width;
}

int common_hal_sharpdisplay_framebuffer_get_height(sharpdisplay_framebuffer_obj_t *self) {
    return self->height;
}

static int common_hal_sharpdisplay_framebuffer_get_row_stride(sharpdisplay_framebuffer_obj_t *self) {
    if (self->jdi_display) {
        return (self->width + 1) / 2 + 2;
    } else {
        return (self->width + 7) / 8 + 2;
    }
}

static int common_hal_sharpdisplay_framebuffer_get_first_pixel_offset(sharpdisplay_framebuffer_obj_t *self) {
    return 2;
}

static bool common_hal_sharpdisplay_framebuffer_get_reverse_pixels_in_byte(sharpdisplay_framebuffer_obj_t *self) {
    return true;
}

static bool common_hal_sharpdisplay_framebuffer_get_pixels_in_byte_share_row(sharpdisplay_framebuffer_obj_t *self) {
    return true;
}

void common_hal_sharpdisplay_framebuffer_reset(sharpdisplay_framebuffer_obj_t *self) {
    if (self->bus != &self->inline_bus
        #if CIRCUITPY_BOARD_SPI
        && !common_hal_board_is_spi(self->bus)
        #endif
        ) {
        memcpy(&self->inline_bus, self->bus, sizeof(busio_spi_obj_t));
        self->bus = &self->inline_bus;
    }
}

void common_hal_sharpdisplay_framebuffer_reconstruct(sharpdisplay_framebuffer_obj_t *self) {
}

void common_hal_sharpdisplay_framebuffer_get_bufinfo(sharpdisplay_framebuffer_obj_t *self, mp_buffer_info_t *bufinfo) {
    if (!self->bufinfo.buf) {
        int row_stride = common_hal_sharpdisplay_framebuffer_get_row_stride(self);
        int height = common_hal_sharpdisplay_framebuffer_get_height(self);
        self->bufinfo.len = row_stride * height + 2;
        self->bufinfo.buf = port_malloc(self->bufinfo.len, false);
        if (self->bufinfo.buf == NULL) {
            m_malloc_fail(self->bufinfo.len);
        }
        memset(self->bufinfo.buf, 0, self->bufinfo.len);

        uint8_t *data = self->bufinfo.buf;
        if (self->jdi_display) {
            *data++ = JDI_BIT_WRITECMD_LSB;
        } else {
            *data++ = SHARPMEM_BIT_WRITECMD_LSB;
        }

        for (int y = 0; y < height; y++) {
            if (self->jdi_display) {
                *data = y + 1;
            } else {
                *data = bitrev(y + 1);
            }
            data += row_stride;
        }
        self->full_refresh = true;
    }
    if (bufinfo) {
        *bufinfo = self->bufinfo;
    }
}

void common_hal_sharpdisplay_framebuffer_deinit(sharpdisplay_framebuffer_obj_t *self) {
    if (self->base.type != &sharpdisplay_framebuffer_type) {
        return;
    }

    if (self->bus == &self->inline_bus) {
        common_hal_busio_spi_deinit(self->bus);
    }

    common_hal_reset_pin(self->chip_select.pin);

    port_free(self->bufinfo.buf);

    memset(self, 0, sizeof(*self));
}

void common_hal_sharpdisplay_framebuffer_construct(
    sharpdisplay_framebuffer_obj_t *self,
    busio_spi_obj_t *spi,
    const mcu_pin_obj_t *chip_select,
    int baudrate,
    int width,
    int height,
    bool jdi_display) {
    common_hal_digitalio_digitalinout_construct(&self->chip_select, chip_select);
    common_hal_digitalio_digitalinout_switch_to_output(&self->chip_select, true, DRIVE_MODE_PUSH_PULL);
    common_hal_never_reset_pin(chip_select);

    self->bus = spi;
    common_hal_busio_spi_never_reset(self->bus);

    self->width = width;
    self->height = height;
    self->baudrate = baudrate;
    self->jdi_display = jdi_display;

    common_hal_sharpdisplay_framebuffer_get_bufinfo(self, NULL);
}

static void common_hal_sharpdisplay_framebuffer_swapbuffers(sharpdisplay_framebuffer_obj_t *self, uint8_t *dirty_row_bitmask) {
    // claim SPI bus
    if (!common_hal_busio_spi_try_lock(self->bus)) {
        return;
    }
    common_hal_busio_spi_configure(self->bus, self->baudrate, 0, 0, 8);

    // set chip select high
    common_hal_digitalio_digitalinout_set_value(&self->chip_select, true);

    // output the toggling signal
    uint8_t *data = self->bufinfo.buf;
    data[0] ^= SHARPMEM_BIT_VCOM_LSB;

    common_hal_busio_spi_write(self->bus, data++, 1);

    // output each changed row
    size_t row_stride = common_hal_sharpdisplay_framebuffer_get_row_stride(self);
    for (int y = 0; y < self->height; y++) {
        if (self->full_refresh || (dirty_row_bitmask[y / 8] & (1 << (y & 7)))) {
            common_hal_busio_spi_write(self->bus, data, row_stride);
        }
        data += row_stride;
    }

    // output a trailing zero
    uint8_t zero[2] = {0, 0};
    common_hal_busio_spi_write(self->bus, zero, self->jdi_display ? 2 : 1);

    // set chip select low
    common_hal_digitalio_digitalinout_set_value(&self->chip_select, false);

    // release SPI bus
    common_hal_busio_spi_unlock(self->bus);

    self->full_refresh = false;
}

static void sharpdisplay_framebuffer_deinit(mp_obj_t self_in) {
    sharpdisplay_framebuffer_obj_t *self = self_in;
    common_hal_sharpdisplay_framebuffer_deinit(self);
}

static void sharpdisplay_framebuffer_get_bufinfo(mp_obj_t self_in, mp_buffer_info_t *bufinfo) {
    sharpdisplay_framebuffer_obj_t *self = self_in;
    common_hal_sharpdisplay_framebuffer_get_bufinfo(self, bufinfo);
}

static int sharpdisplay_framebuffer_get_color_depth(mp_obj_t self_in) {
    sharpdisplay_framebuffer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return self->jdi_display ? 4 : 1;
}

static bool sharpdisplay_framebuffer_get_grayscale(mp_obj_t self_in) {
    sharpdisplay_framebuffer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return !self->jdi_display;
}

static int sharpdisplay_framebuffer_get_height(mp_obj_t self_in) {
    sharpdisplay_framebuffer_obj_t *self = self_in;
    return common_hal_sharpdisplay_framebuffer_get_height(self);
}

static int sharpdisplay_framebuffer_get_width(mp_obj_t self_in) {
    sharpdisplay_framebuffer_obj_t *self = self_in;
    return common_hal_sharpdisplay_framebuffer_get_width(self);
}

static int sharpdisplay_framebuffer_get_first_pixel_offset(mp_obj_t self_in) {
    sharpdisplay_framebuffer_obj_t *self = self_in;
    return common_hal_sharpdisplay_framebuffer_get_first_pixel_offset(self);
}

static bool sharpdisplay_framebuffer_get_pixels_in_byte_share_row(mp_obj_t self_in) {
    sharpdisplay_framebuffer_obj_t *self = self_in;
    return common_hal_sharpdisplay_framebuffer_get_pixels_in_byte_share_row(self);
}

static bool sharpdisplay_framebuffer_get_reverse_pixels_in_byte(mp_obj_t self_in) {
    sharpdisplay_framebuffer_obj_t *self = self_in;
    return common_hal_sharpdisplay_framebuffer_get_reverse_pixels_in_byte(self);
}

static int sharpdisplay_framebuffer_get_row_stride(mp_obj_t self_in) {
    sharpdisplay_framebuffer_obj_t *self = self_in;
    return common_hal_sharpdisplay_framebuffer_get_row_stride(self);
}

static void sharpdisplay_framebuffer_swapbuffers(mp_obj_t self_in, uint8_t *dirty_row_bitmask) {
    sharpdisplay_framebuffer_obj_t *self = self_in;
    common_hal_sharpdisplay_framebuffer_swapbuffers(self, dirty_row_bitmask);
}

const framebuffer_p_t sharpdisplay_framebuffer_proto = {
    MP_PROTO_IMPLEMENT(MP_QSTR_protocol_framebuffer)
    .deinit = sharpdisplay_framebuffer_deinit,
    .get_bufinfo = sharpdisplay_framebuffer_get_bufinfo,
    .get_color_depth = sharpdisplay_framebuffer_get_color_depth,
    .get_grayscale = sharpdisplay_framebuffer_get_grayscale,
    .get_height = sharpdisplay_framebuffer_get_height,
    .get_width = sharpdisplay_framebuffer_get_width,
    .swapbuffers = sharpdisplay_framebuffer_swapbuffers,

    .get_first_pixel_offset = sharpdisplay_framebuffer_get_first_pixel_offset,
    .get_pixels_in_byte_share_row = sharpdisplay_framebuffer_get_pixels_in_byte_share_row,
    .get_reverse_pixels_in_byte = sharpdisplay_framebuffer_get_reverse_pixels_in_byte,
    .get_row_stride = sharpdisplay_framebuffer_get_row_stride,
};

void common_hal_sharpdisplay_framebuffer_collect_ptrs(sharpdisplay_framebuffer_obj_t *self) {
    gc_collect_ptr(self->bus);
}
