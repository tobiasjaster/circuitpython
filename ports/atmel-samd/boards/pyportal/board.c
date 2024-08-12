// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "supervisor/board.h"
#include "mpconfigboard.h"
#include "hal/include/hal_gpio.h"

#include "shared-module/displayio/__init__.h"
#include "shared-module/displayio/mipi_constants.h"

#define DELAY 0x80

uint8_t display_init_sequence[] = {
    0xEF, 3, 0x03, 0x80, 0x02,
    0xCF, 3, 0x00, 0xC1, 0x30,
    0xED, 4, 0x64, 0x03, 0x12, 0x81,
    0xE8, 3, 0x85, 0x00, 0x78,
    0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
    0xF7, 1, 0x20,
    0xEA, 2, 0x00, 0x00,
    0xc0, 1, 0x23,             // Power control VRH[5:0]
    0xc1, 1, 0x10,             // Power control SAP[2:0];BT[3:0]
    0xc5, 2, 0x3e, 0x28,       // VCM control
    0xc7, 1, 0x86,             // VCM control2
    0x36, 1, 0xa8,             // Memory Access Control
    0x37, 1, 0x00,             // Vertical scroll zero
    0x3a, 1, 0x55,             // COLMOD: Pixel Format Set
    0xb1, 2, 0x00, 0x18,       // Frame Rate Control (In Normal Mode/Full Colors)
    0xb6, 3, 0x08, 0xa2, 0x27, // Display Function Control
    0xF2, 1, 0x00,                         // 3Gamma Function Disable
    0x26, 1, 0x01,             // Gamma curve selected
    0xe0, 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, // Set Gamma
    0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
    0xe1, 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, // Set Gamma
    0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
    0x11, DELAY, 120,                // Exit Sleep
    0x29, DELAY, 120, // Display on
};

void board_init(void) {
    paralleldisplaybus_parallelbus_obj_t *bus = &allocate_display_bus()->parallel_bus;
    bus->base.type = &paralleldisplaybus_parallelbus_type;
    common_hal_paralleldisplaybus_parallelbus_construct(bus,
        &pin_PA16, // Data0
        &pin_PB05, // Command or data
        &pin_PB06, // Chip select
        &pin_PB09, // Write
        &pin_PB04, // Read
        &pin_PA00, // Reset
        0); // Frequency

    busdisplay_busdisplay_obj_t *display = &allocate_display()->display;
    display->base.type = &busdisplay_busdisplay_type;
    common_hal_busdisplay_busdisplay_construct(display,
        bus,
        320, // Width
        240, // Height
        0, // column start
        0, // row start
        0, // rotation
        16, // Color depth
        false, // grayscale
        false, // pixels_in_byte_share_row (unused for depths > 8)
        1, // bytes per cell. Only valid for depths < 8
        false, // reverse_pixels_in_byte. Only valid for depths < 8
        true, // reverse_pixels_in_word
        MIPI_COMMAND_SET_COLUMN_ADDRESS, // Set column command
        MIPI_COMMAND_SET_PAGE_ADDRESS, // Set row command
        MIPI_COMMAND_WRITE_MEMORY_START, // Write memory command
        display_init_sequence,
        sizeof(display_init_sequence),
        &pin_PB31, // Backlight pin
        NO_BRIGHTNESS_COMMAND,
        1.0f, // brightness
        false, // single_byte_bounds
        false, // data_as_commands
        true, // auto_refresh
        60, // native_frames_per_second
        true, // backlight_on_high
        false, // SH1107_addressing
        50000); // backlight pwm frequency
}

// Use the MP_WEAK supervisor/shared/board.c versions of routines not defined here.
