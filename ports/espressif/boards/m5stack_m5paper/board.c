// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 fonix232
//
// SPDX-License-Identifier: MIT

#include "supervisor/board.h"

#include "mpconfigboard.h"
#include "shared-bindings/busio/SPI.h"
#include "shared-bindings/fourwire/FourWire.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-module/displayio/__init__.h"
#include "shared-bindings/board/__init__.h"


const uint8_t display_start_sequence[] = { };

const uint8_t display_stop_sequence[] = { };

const uint8_t display_refresh_sequence[] = { };


void board_init(void) {

    // TODO: Investigate how to initialise display

    // // Set up the SPI object used to control the display
    // busio_spi_obj_t *spi = common_hal_board_create_spi(0);
    // common_hal_busio_spi_never_reset(spi);

    // // Set up the DisplayIO pin object
    // fourwire_fourwire_obj_t *bus = &allocate_display_bus()->fourwire_bus;
    // bus->base.type = &fourwire_fourwire_type;
    // common_hal_fourwire_fourwire_construct(bus,
    //     spi,
    //     &pin_GPIO20, // EPD_DC Command or data
    //     &pin_GPIO15, // EPD_CS Chip select
    //     &pin_GPIO23, // EPD_RST Reset
    //     1200000, // Baudrate
    //     0, // Polarity
    //     0); // Phase

    // epaperdisplay_epaperdisplay_obj_t *display = &allocate_display()->epaper_display;
    // display->base.type = &epaperdisplay_epaperdisplay_type;

    // common_hal_epaperdisplay_epaperdisplay_construct(
    //     display,
    //     bus,
    //     display_start_sequence, sizeof(display_start_sequence),
    //     1.0, // start up time
    //     display_stop_sequence, sizeof(display_stop_sequence),
    //     540,  // width
    //     960,  // height
    //     540,  // ram_width
    //     960,  // ram_height
    //     0,  // colstart
    //     0,  // rowstart
    //     90,  // rotation
    //     NO_COMMAND,  // set_column_window_command
    //     NO_COMMAND,  // set_row_window_command
    //     NO_COMMAND,  // set_current_column_command
    //     NO_COMMAND,  // set_current_row_command
    //     NO_COMMAND,  // write_black_ram_command
    //     false,  // black_bits_inverted
    //     NO_COMMAND,  // write_color_ram_command
    //     false,  // color_bits_inverted
    //     0x000000,  // highlight_color
    //     refresh_sequence, sizeof(refresh_sequence),
    //     28.0,  // refresh_time
    //     &pin_GPIO27,  // busy_pin
    //     false,  // busy_state
    //     30.0, // seconds_per_frame
    //     false,  // always_toggle_chip_select
    //     false, // grayscale
    //     true, // acep
    //     false,  // two_byte_sequence_length
    //     false  // address_little_endian
    // );
}
