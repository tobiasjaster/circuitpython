// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2024 Adafruit Industries LLC
//
// SPDX-License-Identifier: MIT

#pragma once

#define MICROPY_HW_BOARD_NAME       "LILYGO TTGO T-01C3"
#define MICROPY_HW_MCU_NAME         "ESP32-C3"

#define MICROPY_HW_LED_STATUS       (&pin_GPIO3)

#define DEFAULT_UART_BUS_RX         (&pin_GPIO20)
#define DEFAULT_UART_BUS_TX         (&pin_GPIO21)

#define CIRCUITPY_CONSOLE_UART_RX     DEFAULT_UART_BUS_RX
#define CIRCUITPY_CONSOLE_UART_TX     DEFAULT_UART_BUS_TX
