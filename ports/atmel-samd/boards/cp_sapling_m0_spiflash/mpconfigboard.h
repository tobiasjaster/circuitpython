// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#define MICROPY_HW_BOARD_NAME "CP Sapling M0 w/ SPI Flash"
#define MICROPY_HW_MCU_NAME "samd21e18"

#define MICROPY_HW_NEOPIXEL (&pin_PA15)

#define SPI_FLASH_MOSI_PIN          &pin_PA18
#define SPI_FLASH_MISO_PIN          &pin_PA17
#define SPI_FLASH_SCK_PIN           &pin_PA19
#define SPI_FLASH_CS_PIN            &pin_PA22

#define IGNORE_PIN_PA02     1
#define IGNORE_PIN_PA03     1
#define IGNORE_PIN_PA04     1
#define IGNORE_PIN_PA05     1
#define IGNORE_PIN_PA06     1
#define IGNORE_PIN_PA07     1
#define IGNORE_PIN_PA12     1
#define IGNORE_PIN_PA13     1
#define IGNORE_PIN_PA14     1
#define IGNORE_PIN_PA20     1
#define IGNORE_PIN_PA21     1
// USB is always used internally so skip the pin objects for it.
#define IGNORE_PIN_PA24     1
#define IGNORE_PIN_PA25     1
#define IGNORE_PIN_PA27     1
#define IGNORE_PIN_PA28     1
#define IGNORE_PIN_PA30     1
#define IGNORE_PIN_PA31     1
#define IGNORE_PIN_PB01     1
#define IGNORE_PIN_PB02     1
#define IGNORE_PIN_PB03     1
#define IGNORE_PIN_PB04     1
#define IGNORE_PIN_PB05     1
#define IGNORE_PIN_PB06     1
#define IGNORE_PIN_PB07     1
#define IGNORE_PIN_PB08     1
#define IGNORE_PIN_PB09     1
#define IGNORE_PIN_PB10     1
#define IGNORE_PIN_PB11     1
#define IGNORE_PIN_PB12     1
#define IGNORE_PIN_PB13     1
#define IGNORE_PIN_PB14     1
#define IGNORE_PIN_PB15     1
#define IGNORE_PIN_PB16     1
#define IGNORE_PIN_PB17     1
#define IGNORE_PIN_PB22     1
#define IGNORE_PIN_PB23     1
#define IGNORE_PIN_PB30     1
#define IGNORE_PIN_PB31     1
#define IGNORE_PIN_PB00     1

#define DEFAULT_I2C_BUS_SCL (&pin_PA09)
#define DEFAULT_I2C_BUS_SDA (&pin_PA08)

#define DEFAULT_SPI_BUS_SS (&pin_PA22)
#define DEFAULT_SPI_BUS_SCK (&pin_PA19)
#define DEFAULT_SPI_BUS_MOSI (&pin_PA18)
#define DEFAULT_SPI_BUS_MISO (&pin_PA17)
