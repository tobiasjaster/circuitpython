// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Joey Castillo
//
// SPDX-License-Identifier: MIT

#include "supervisor/board.h"
#include "mpconfigboard.h"
#include "hal/include/hal_gpio.h"
#include "shared-bindings/busio/SPI.h"
#include "shared-bindings/fourwire/FourWire.h"
#include "shared-bindings/time/__init__.h"
#include "shared-module/displayio/__init__.h"
#include "shared-module/displayio/mipi_constants.h"


#define DELAY 0x80
#define HEIGHT 400
#define WIDTH 300

uint8_t start_sequence[] = {
    0x01, 0x04, 0x03, 0x00, 0x2b, 0x2b, // power setting
    0x06, 0x03, 0x17, 0x17, 0x17, // booster soft start
    0x04, 0x80, 0xc8, // power on and wait 200 ms
    0x00, 0x01, 0x0f, // panel setting
    0x61, 0x04, (HEIGHT >> 8) & 0xFF, HEIGHT & 0xFF, (WIDTH >> 8) & 0xFF, WIDTH & 0xFF // Resolution
};

uint8_t stop_sequence[] = {
    0x50, 0x01, 0xf7, // CDI setting
    0x02, 0x80, 0xf0  // Power off
};

uint8_t refresh_sequence[] = {
    0x12, 0x00
};

void board_init(void) {
    fourwire_fourwire_obj_t *bus = &allocate_display_bus()->fourwire_bus;
    busio_spi_obj_t *spi = &bus->inline_bus;
    common_hal_busio_spi_construct(spi, &pin_PB13, &pin_PB15, NULL, false);
    common_hal_busio_spi_never_reset(spi);

    bus->base.type = &fourwire_fourwire_type;
    common_hal_fourwire_fourwire_construct(bus,
        spi,
        &pin_PB05, // EPD_DC Command or data
        &pin_PB07, // EPD_CS Chip select
        &pin_PA00, // EPD_RST Reset
        1000000, // Baudrate
        0, // Polarity
        0); // Phase

    epaperdisplay_epaperdisplay_obj_t *display = &allocate_display()->epaper_display;
    display->base.type = &epaperdisplay_epaperdisplay_type;
    common_hal_epaperdisplay_epaperdisplay_construct(display,
        bus,
        start_sequence,
        sizeof(start_sequence),
        0, // start up time
        stop_sequence,
        sizeof(stop_sequence),
        300, // width
        400, // height
        300, // RAM width
        400, // RAM height
        0, // colstart
        0, // rowstart
        270, // rotation
        NO_COMMAND, // set_column_window_command
        NO_COMMAND, // set_row_window_command
        NO_COMMAND, // set_current_column_command
        NO_COMMAND, // set_current_row_command
        0x13, // write_black_ram_command
        false, // black_bits_inverted
        NO_COMMAND, // write_color_ram_command (can add this for grayscale eventually)
        false, // color_bits_inverted
        0x000000, // highlight_color
        refresh_sequence, // refresh_display_sequence
        sizeof(refresh_sequence),
        40, // refresh_time
        &pin_PA01, // busy_pin
        false, // busy_state
        5, // seconds_per_frame
        false, // chip_select (don't always toggle chip select)
        false, // grayscale
        false, // acep
        false, // two_byte_sequence_length
        false); // address_little_endian
}

// Use the MP_WEAK supervisor/shared/board.c versions of routines not defined here.
