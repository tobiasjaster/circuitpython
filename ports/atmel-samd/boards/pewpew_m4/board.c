// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries, 2020 Radomir
//
// SPDX-License-Identifier: MIT

#include "supervisor/board.h"
#include "mpconfigboard.h"
#include "hal/include/hal_gpio.h"
#include "shared-bindings/busio/SPI.h"
#include "shared-bindings/fourwire/FourWire.h"
#include "shared-module/displayio/__init__.h"
#include "shared-module/displayio/mipi_constants.h"


typedef struct {
    const uint32_t *config_data;
    void *handoverHID;
    void *handoverMSC;
    const char *info_uf2;
} UF2_BInfo;

#define APP_START_ADDRESS 0x00004000
#define UF2_BINFO ((UF2_BInfo *)(APP_START_ADDRESS - sizeof(UF2_BInfo)))

#define CFG_DISPLAY_CFG0 39
#define CFG_MAGIC0 0x1e9e10f1

#define DELAY 0x80

static uint32_t lookupCfg(uint32_t key, uint32_t defl) {
    const uint32_t *ptr = UF2_BINFO->config_data;
    if (!ptr || (((uint32_t)ptr) & 3) || *ptr != CFG_MAGIC0) {
        // no config data!
    } else {
        ptr += 4;
        while (*ptr) {
            if (*ptr == key) {
                return ptr[1];
            }
            ptr += 2;
        }
    }
    return defl;
}

uint8_t display_init_sequence[] = {
    0x01, 0 | DELAY, 150, // SWRESET
    0x11, 0 | DELAY, 255, // SLPOUT
    0xb1, 3, 0x01, 0x2C, 0x2D, // _FRMCTR1
    0xb2, 3, 0x01, 0x2C, 0x2D, //
    0xb3, 6, 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D,
    0xb4, 1, 0x07, // _INVCTR line inversion
    0xc0, 3, 0xa2, 0x02, 0x84, // _PWCTR1 GVDD = 4.7V, 1.0uA
    0xc1, 1, 0xc5, // _PWCTR2 VGH=14.7V, VGL=-7.35V
    0xc2, 2, 0x0a, 0x00, // _PWCTR3 Opamp current small, Boost frequency
    0xc3, 2, 0x8a, 0x2a,
    0xc4, 2, 0x8a, 0xee,
    0xc5, 1, 0x0e, // _VMCTR1 VCOMH = 4V, VOML = -1.1V
    0x2a, 0, // _INVOFF
    0x36, 1, 0xa0, // _MADCTL
    // 1 clk cycle nonoverlap, 2 cycle gate rise, 3 cycle osc equalie,
    // fix on VTL
    0x3a, 1, 0x05, // COLMOD - 16bit color
    0xe0, 16, 0x02, 0x1c, 0x07, 0x12, // _GMCTRP1 Gamma
    0x37, 0x32, 0x29, 0x2d,
    0x29, 0x25, 0x2B, 0x39,
    0x00, 0x01, 0x03, 0x10,
    0xe1, 16, 0x03, 0x1d, 0x07, 0x06, // _GMCTRN1
    0x2E, 0x2C, 0x29, 0x2D,
    0x2E, 0x2E, 0x37, 0x3F,
    0x00, 0x00, 0x02, 0x10,
    0x13, 0 | DELAY, 10, // _NORON
    0x29, 0 | DELAY, 100, // _DISPON
};

void board_init(void) {
    fourwire_fourwire_obj_t *bus = &allocate_display_bus()->fourwire_bus;
    busio_spi_obj_t *spi = &bus->inline_bus;
    common_hal_busio_spi_construct(spi, &pin_PA13, &pin_PA15, NULL, false);
    common_hal_busio_spi_never_reset(spi);

    bus->base.type = &fourwire_fourwire_type;
    common_hal_fourwire_fourwire_construct(bus,
        spi,
        &pin_PA16, // TFT_DC Command or data
        &pin_PA11, // TFT_CS Chip select
        &pin_PA17, // TFT_RST Reset
        60000000, // Baudrate
        0, // Polarity
        0); // Phase

    uint32_t cfg0 = lookupCfg(CFG_DISPLAY_CFG0, 0x000000);
    uint32_t offX = (cfg0 >> 8) & 0xff;
    uint32_t offY = (cfg0 >> 16) & 0xff;
    busdisplay_busdisplay_obj_t *display = &allocate_display()->display;
    display->base.type = &busdisplay_busdisplay_type;
    common_hal_busdisplay_busdisplay_construct(display,
        bus,
        160, // Width (after rotation)
        128, // Height (after rotation)
        offX, // column start
        offY, // row start
        0, // rotation
        16, // Color depth
        false, // grayscale
        false, // pixels in byte share row. only used for depth < 8
        1, // bytes per cell. Only valid for depths < 8
        false, // reverse_pixels_in_byte. Only valid for depths < 8
        true, // reverse_pixels_in_word
        MIPI_COMMAND_SET_COLUMN_ADDRESS, // Set column command
        MIPI_COMMAND_SET_PAGE_ADDRESS, // Set row command
        MIPI_COMMAND_WRITE_MEMORY_START, // Write memory command
        display_init_sequence,
        sizeof(display_init_sequence),
        NULL,  // backlight pin
        NO_BRIGHTNESS_COMMAND,
        1.0f, // brightness
        false, // single_byte_bounds
        false, // data_as_commands
        false, // auto_refresh
        20, // native_frames_per_second
        true, // backlight_on_high
        false, // SH1107_addressing
        50000); // backlight pwm frequency
}

// Use the MP_WEAK supervisor/shared/board.c versions of routines not defined here.
