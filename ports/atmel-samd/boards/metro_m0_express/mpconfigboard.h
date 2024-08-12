// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#define MICROPY_HW_BOARD_NAME "Adafruit Metro M0 Express"
#define MICROPY_HW_MCU_NAME "samd21g18"

#define MICROPY_HW_LED_STATUS   (&pin_PA17)
#define MICROPY_HW_LED_TX   &pin_PA27
// Comment this out if you have trouble connecting over SWD. It's one of the SWD pins.
#define MICROPY_HW_LED_RX   &pin_PA31

#define MICROPY_HW_NEOPIXEL (&pin_PA30)

// Clock rates are off: Saleae reads 12MHz which is the limit even though we set it to the safer 8MHz.
#define SPI_FLASH_BAUDRATE  (8000000)

#define SPI_FLASH_MOSI_PIN          &pin_PB22
#define SPI_FLASH_MISO_PIN          &pin_PB03
#define SPI_FLASH_SCK_PIN           &pin_PB23
#define SPI_FLASH_CS_PIN            &pin_PA13

#define BOARD_HAS_CRYSTAL 1

#define DEFAULT_I2C_BUS_SCL (&pin_PA23)
#define DEFAULT_I2C_BUS_SDA (&pin_PA22)

#define DEFAULT_SPI_BUS_SCK (&pin_PB11)
#define DEFAULT_SPI_BUS_MOSI (&pin_PB10)
#define DEFAULT_SPI_BUS_MISO (&pin_PA12)

#define DEFAULT_UART_BUS_RX (&pin_PA11)
#define DEFAULT_UART_BUS_TX (&pin_PA10)

// These pins are connected to the external crystal.
#define IGNORE_PIN_PA00     1
#define IGNORE_PIN_PA01     1

// USB is always used internally so skip the pin objects for it.
#define IGNORE_PIN_PA24     1
#define IGNORE_PIN_PA25     1
