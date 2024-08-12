// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "supervisor/board.h"

#include "mpconfigboard.h"
#include "shared-bindings/busio/SPI.h"
#include "shared-bindings/fourwire/FourWire.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-module/displayio/__init__.h"
#include "supervisor/shared/board.h"

#include "components/log/include/esp_log.h"

static const char *TAG = "board";

#define DELAY 0x80

// This is an ILO373 control chip. The display is a 2.9" grayscale EInk.

const uint8_t display_start_sequence[] = {
    0x01, 5, 0x03, 0x00, 0x2b, 0x2b, 0x13, // power setting
    0x06, 3, 0x17, 0x17, 0x17, // booster soft start
    0x04, DELAY, 200, // power on and wait 200 ms
    0x00, 1, 0x7f, // panel setting
    0x50, 1, 0x97, // CDI setting
    0x30, 1, 0x3c, // PLL set to 50 Hx (M = 7, N = 4)
    0x61, 3, 0x80, 0x01, 0x28, // Resolution
    0x82, DELAY | 1, 0x12, 50, // VCM DC and delay 50ms

    // Look up tables for voltage sequence for pixel transition
    // Common voltage
    0x20, 0x2a,
    0x00, 0x0a, 0x00, 0x00, 0x00, 0x01,
    0x60, 0x14, 0x14, 0x00, 0x00, 0x01,
    0x00, 0x14, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x13, 0x0a, 0x01, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // White to white
    0x21, 0x2a,
    0x40, 0x0a, 0x00, 0x00, 0x00, 0x01,
    0x90, 0x14, 0x14, 0x00, 0x00, 0x01,
    0x10, 0x14, 0x0a, 0x00, 0x00, 0x01,
    0xa0, 0x13, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Black to white
    0x22, 0x2a,
    0x40, 0x0a, 0x00, 0x00, 0x00, 0x01,
    0x90, 0x14, 0x14, 0x00, 0x00, 0x01,
    0x00, 0x14, 0x0a, 0x00, 0x00, 0x01,
    0x99, 0x0c, 0x01, 0x03, 0x04, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // White to black
    0x23, 0x2a,
    0x40, 0x0a, 0x00, 0x00, 0x00, 0x01,
    0x90, 0x14, 0x14, 0x00, 0x00, 0x01,
    0x00, 0x14, 0x0a, 0x00, 0x00, 0x01,
    0x99, 0x0b, 0x04, 0x04, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Black to black
    0x24, 0x2a,
    0x80, 0x0a, 0x00, 0x00, 0x00, 0x01,
    0x90, 0x14, 0x14, 0x00, 0x00, 0x01,
    0x20, 0x14, 0x0a, 0x00, 0x00, 0x01,
    0x50, 0x13, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t display_stop_sequence[] = {
    0x50, 0x01, 0x17,  // CDI Setting
    0x82, 0x01, 0x00,  // VCM DC to -0.1V
    0x02, 0x00  // Power off
};

const uint8_t refresh_sequence[] = {
    0x12, 0x00
};

void board_init(void) {
    fourwire_fourwire_obj_t *bus = &allocate_display_bus()->fourwire_bus;
    busio_spi_obj_t *spi = &bus->inline_bus;
    common_hal_busio_spi_construct(spi, &pin_GPIO36, &pin_GPIO35, NULL, false);
    common_hal_busio_spi_never_reset(spi);

    bus->base.type = &fourwire_fourwire_type;
    common_hal_fourwire_fourwire_construct(bus,
        spi,
        &pin_GPIO7, // EPD_DC Command or data
        &pin_GPIO8, // EPD_CS Chip select
        &pin_GPIO6, // EPD_RST Reset
        4000000, // Baudrate
        0, // Polarity
        0); // Phase

    epaperdisplay_epaperdisplay_obj_t *display = &allocate_display()->epaper_display;
    display->base.type = &epaperdisplay_epaperdisplay_type;
    common_hal_epaperdisplay_epaperdisplay_construct(
        display,
        bus,
        display_start_sequence, sizeof(display_start_sequence),
        0, // start up time
        display_stop_sequence, sizeof(display_stop_sequence),
        296,  // width
        128,  // height
        160,  // ram_width
        296,  // ram_height
        0,  // colstart
        0,  // rowstart
        270,  // rotation
        NO_COMMAND,  // set_column_window_command
        NO_COMMAND,  // set_row_window_command
        NO_COMMAND,  // set_current_column_command
        NO_COMMAND,  // set_current_row_command
        0x10,  // write_black_ram_command
        false,  // black_bits_inverted
        0x13,  // write_color_ram_command
        false,  // color_bits_inverted
        0x000000,  // highlight_color
        refresh_sequence, sizeof(refresh_sequence),
        1.0,  // refresh_time
        &pin_GPIO5,  // busy_pin
        false,  // busy_state
        5.0, // seconds_per_frame
        false,  // always_toggle_chip_select
        true, // grayscale
        false, // acep
        false,  // two_byte_sequence_length
        false); // address_little_endian
}

bool espressif_board_reset_pin_number(gpio_num_t pin_number) {
    // Pin 16 is speaker enable and it's pulled down on the board. We don't want
    // to pull it high because then we'll compete with the external pull down.
    // So, reset without any pulls internally.
    if (pin_number == 16) {
        gpio_config_t cfg = {
            .pin_bit_mask = BIT64(16),
            .mode = GPIO_MODE_DISABLE,
            // The pin is externally pulled down, so we don't need to pull it.
            .pull_up_en = false,
            .pull_down_en = false,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&cfg);
        return true;
    }
    // Pin 4 is used for voltage monitoring, so don't reset
    if (pin_number == 4) {
        return true;
    }
    return false;
}

void board_deinit(void) {
    epaperdisplay_epaperdisplay_obj_t *display = &displays[0].epaper_display;
    if (display->base.type == &epaperdisplay_epaperdisplay_type) {
        size_t i = 0;
        while (common_hal_epaperdisplay_epaperdisplay_get_busy(display)) {
            RUN_BACKGROUND_TASKS;
            i++;
        }
        ESP_LOGI(TAG, "waited %d iterations for display", i);
    } else {
        ESP_LOGI(TAG, "didn't wait for display");
    }
    common_hal_displayio_release_displays();
}

// Use the MP_WEAK supervisor/shared/board.c versions of routines not defined here.
