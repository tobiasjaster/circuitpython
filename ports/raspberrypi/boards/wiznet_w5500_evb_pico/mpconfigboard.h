// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#define MICROPY_HW_BOARD_NAME "W5500-EVB-Pico"
#define MICROPY_HW_MCU_NAME "rp2040"

#define DEFAULT_SPI_BUS_SCK (&pin_GPIO18)
#define DEFAULT_SPI_BUS_MOSI (&pin_GPIO19)
#define DEFAULT_SPI_BUS_MISO (&pin_GPIO16)

#define DEFAULT_UART_BUS_RX (&pin_GPIO1)
#define DEFAULT_UART_BUS_TX (&pin_GPIO0)
