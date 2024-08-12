// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Scott Shawcroft for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2019 Artur Pacholec
//
// SPDX-License-Identifier: MIT

#pragma once

#define MICROPY_HW_BOARD_NAME "SparkFun Teensy MicroMod Processor"
#define MICROPY_HW_MCU_NAME "IMXRT1062DVL6A"

// If you change this, then make sure to update the linker scripts as well to
// make sure you don't overwrite code
#define CIRCUITPY_INTERNAL_NVM_SIZE 0

#define BOARD_FLASH_SIZE (16 * 1024 * 1024)

#define DEFAULT_I2C_BUS_SCL (&pin_GPIO_AD_B1_00)
#define DEFAULT_I2C_BUS_SDA (&pin_GPIO_AD_B1_01)

#define DEFAULT_SPI_BUS_SCK  (&pin_GPIO_B0_03)
#define DEFAULT_SPI_BUS_MOSI (&pin_GPIO_B0_02)
#define DEFAULT_SPI_BUS_MISO (&pin_GPIO_B0_01)

#define DEFAULT_UART_BUS_RX (&pin_GPIO_AD_B0_03)
#define DEFAULT_UART_BUS_TX (&pin_GPIO_AD_B0_02)

#define CIRCUITPY_USB_DEVICE_INSTANCE 0
#define CIRCUITPY_USB_HOST_INSTANCE 1
