// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#include "nrfx/hal/nrf_gpio.h"

#define MICROPY_HW_BOARD_NAME       "Arduino Nano 33 BLE Rev2"
#define MICROPY_HW_MCU_NAME         "nRF52840"

#define DEFAULT_I2C_BUS_SCL         (&pin_P0_02)
#define DEFAULT_I2C_BUS_SDA         (&pin_P0_31)

#define DEFAULT_SPI_BUS_SCK         (&pin_P0_13)
#define DEFAULT_SPI_BUS_MOSI        (&pin_P0_01)
#define DEFAULT_SPI_BUS_MISO        (&pin_P1_08)

#define DEFAULT_UART_BUS_RX         (&pin_P1_10)
#define DEFAULT_UART_BUS_TX         (&pin_P1_03)
