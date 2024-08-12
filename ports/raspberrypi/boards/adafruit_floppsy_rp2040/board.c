// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "supervisor/board.h"
#include "mpconfigboard.h"

#include "shared-bindings/busio/SPI.h"
#include "shared-bindings/fourwire/FourWire.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-module/displayio/__init__.h"
#include "shared-module/displayio/mipi_constants.h"
#include "shared-bindings/board/__init__.h"


#define DELAY 0x80

uint8_t display_init_sequence[] = {
    0x01, 0 | DELAY, 120, // SWRESET
    0x11, 0 | DELAY, 5, // SLPOUT
    0x36, 1, 0b10100000,  // _MADCTL for rotation 0
    0x3a, 1, 0x55, // COLMOD - 16bit color
    0x21, 0,                 // _INVON
    0x13, 0,                 // _NORON
    0x29, 0 | DELAY, 5,                // _DISPON
};


void board_init(void) {
    busio_spi_obj_t *spi = common_hal_board_create_spi(0);
    fourwire_fourwire_obj_t *bus = &allocate_display_bus()->fourwire_bus;
    bus->base.type = &fourwire_fourwire_type;
    common_hal_fourwire_fourwire_construct(bus,
        spi,
        &pin_GPIO25, // TFT_DC Command or data
        &pin_GPIO29, // TFT_CS Chip select
        NULL, // TFT_RESET Reset
        40000000, // Baudrate
        0, // Polarity
        0); // Phase

    busdisplay_busdisplay_obj_t *display = &allocate_display()->display;
    display->base.type = &busdisplay_busdisplay_type;
    common_hal_busdisplay_busdisplay_construct(
        display,
        bus,
        240, // Width (after rotation)
        240, // Height (after rotation)
        80, // column start
        0, // row start
        0, // rotation
        16, // Color depth
        false, // Grayscale
        false, // Pixels in a byte share a row. Only used for depth < 8
        1, // bytes per cell. Only valid for depths < 8
        false, // reverse_pixels_in_byte. Only valid for depths < 8
        true, // reverse_pixels_in_word
        MIPI_COMMAND_SET_COLUMN_ADDRESS, // Set column command
        MIPI_COMMAND_SET_PAGE_ADDRESS, // Set row command
        MIPI_COMMAND_WRITE_MEMORY_START, // Write memory command
        display_init_sequence,
        sizeof(display_init_sequence),
        &pin_GPIO28,  // backlight pin
        NO_BRIGHTNESS_COMMAND,
        1.0f, // brightness
        false, // single_byte_bounds
        false, // data_as_commands
        true, // auto_refresh
        60, // native_frames_per_second
        true, // backlight_on_high
        false, // not SH1107
        50000); // backlight pwm frequency
}

void board_deinit(void) {
    common_hal_displayio_release_displays();
}

// Use the MP_WEAK supervisor/shared/board.c versions of routines not defined here.
