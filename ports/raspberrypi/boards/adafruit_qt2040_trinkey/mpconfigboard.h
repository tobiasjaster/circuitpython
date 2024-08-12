// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#define MICROPY_HW_BOARD_NAME "Adafruit QT2040 Trinkey"
#define MICROPY_HW_MCU_NAME "rp2040"

#define MICROPY_HW_NEOPIXEL (&pin_GPIO27)

#define DEFAULT_I2C_BUS_SCL (&pin_GPIO17)
#define DEFAULT_I2C_BUS_SDA (&pin_GPIO16)

#define DEFAULT_UART_BUS_RX (&pin_GPIO17)
#define DEFAULT_UART_BUS_TX (&pin_GPIO16)
