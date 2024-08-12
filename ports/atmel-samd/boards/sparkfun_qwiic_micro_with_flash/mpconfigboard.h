// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#define MICROPY_HW_BOARD_NAME "SparkFun Qwiic Micro"
#define MICROPY_HW_MCU_NAME "samd21e18"

// Clock rates are off: Salae reads 12MHz which is the limit even though we set it to the safer 8MHz.
#define SPI_FLASH_BAUDRATE  (8000000)

#define SPI_FLASH_MOSI_PIN   &pin_PA16
#define SPI_FLASH_MISO_PIN   &pin_PA18
#define SPI_FLASH_SCK_PIN    &pin_PA17
#define SPI_FLASH_CS_PIN     &pin_PA19

#define CALIBRATE_CRYSTALLESS 1
#define BOARD_HAS_CRYSTAL 0

// I2C and Qwiic Connector
#define DEFAULT_I2C_BUS_SCL  (&pin_PA09)
#define DEFAULT_I2C_BUS_SDA  (&pin_PA08)

// SPI
#define DEFAULT_SPI_BUS_SCK  (&pin_PA07)
#define DEFAULT_SPI_BUS_MOSI (&pin_PA06)
#define DEFAULT_SPI_BUS_MISO (&pin_PA05)

// UART
#define DEFAULT_UART_BUS_RX  (&pin_PA23)
#define DEFAULT_UART_BUS_TX  (&pin_PA22)

// USB is always used internally so skip the pin objects for it.
#define IGNORE_PIN_PA24     1
#define IGNORE_PIN_PA25     1

// Unused
#define IGNORE_PIN_PA03     1
#define IGNORE_PIN_PA11     1
#define IGNORE_PIN_PA14     1
#define IGNORE_PIN_PA15     1
#define IGNORE_PIN_PA27     1
#define IGNORE_PIN_PA28     1
