// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2018 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#define MICROPY_HW_BOARD_NAME "SAM E54 Xplained Pro"
#define MICROPY_HW_MCU_NAME "same54p20"

#define CIRCUITPY_MCU_FAMILY samd51

// This is for Rev B which is green and has the SD card slot at the edge of the board.

#define MICROPY_HW_LED_STATUS   (&pin_PC18)

#define BOARD_HAS_CRYSTAL 1

#define DEFAULT_I2C_BUS_SCL (&pin_PD09)
#define DEFAULT_I2C_BUS_SDA (&pin_PD08)

#define DEFAULT_SPI_BUS_SCK (&pin_PC05)
#define DEFAULT_SPI_BUS_MOSI (&pin_PC04)
#define DEFAULT_SPI_BUS_MISO (&pin_PC07)

#define DEFAULT_UART_BUS_RX (&pin_PB17)
#define DEFAULT_UART_BUS_TX (&pin_PB16)

// USB is always used internally so skip the pin objects for it.
#define IGNORE_PIN_PA24     1
#define IGNORE_PIN_PA25     1
#define IGNORE_PIN_PC19     1
