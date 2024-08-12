// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/board/__init__.h"
// Version 2.4
static const mp_rom_map_elem_t board_module_globals_table[] = {
    CIRCUITPYTHON_BOARD_DICT_STANDARD_ITEMS

    // SERVO Pins
    { MP_ROM_QSTR(MP_QSTR_SERVO1), MP_ROM_PTR(&pin_PA18) },
    { MP_ROM_QSTR(MP_QSTR_SERVO2), MP_ROM_PTR(&pin_PA19) },
    { MP_ROM_QSTR(MP_QSTR_SERVO3), MP_ROM_PTR(&pin_PA20) },
    { MP_ROM_QSTR(MP_QSTR_SERVO4), MP_ROM_PTR(&pin_PA21) },
    { MP_ROM_QSTR(MP_QSTR_SERVO5), MP_ROM_PTR(&pin_PA11) },
    { MP_ROM_QSTR(MP_QSTR_SERVO6), MP_ROM_PTR(&pin_PA10) },
    { MP_ROM_QSTR(MP_QSTR_SERVO7), MP_ROM_PTR(&pin_PA09) },
    { MP_ROM_QSTR(MP_QSTR_SERVO8), MP_ROM_PTR(&pin_PA08) },

    // RCC Pins
    { MP_ROM_QSTR(MP_QSTR_RCC1), MP_ROM_PTR(&pin_PA07) },
    { MP_ROM_QSTR(MP_QSTR_RCC2), MP_ROM_PTR(&pin_PA06) },
    { MP_ROM_QSTR(MP_QSTR_RCC3), MP_ROM_PTR(&pin_PA05) },
    { MP_ROM_QSTR(MP_QSTR_RCC4), MP_ROM_PTR(&pin_PA04) },

    // Special Function
    { MP_ROM_QSTR(MP_QSTR_VOLTAGE_MONITOR), MP_ROM_PTR(&pin_PA02) },
    { MP_ROM_QSTR(MP_QSTR_BATTERY), MP_ROM_PTR(&pin_PA02) },
    { MP_ROM_QSTR(MP_QSTR_POWER_OFF), MP_ROM_PTR(&pin_PA03) },
    { MP_ROM_QSTR(MP_QSTR_POWER_DISABLE), MP_ROM_PTR(&pin_PA03) },
    { MP_ROM_QSTR(MP_QSTR_POWER_ON), MP_ROM_PTR(&pin_PA27) },
    { MP_ROM_QSTR(MP_QSTR_POWER_ENABLE), MP_ROM_PTR(&pin_PA27) },
    { MP_ROM_QSTR(MP_QSTR_BUTTON), MP_ROM_PTR(&pin_PA27) },

    { MP_ROM_QSTR(MP_QSTR_NEOPIXEL), MP_ROM_PTR(&pin_PB23) },
    { MP_ROM_QSTR(MP_QSTR_LED), MP_ROM_PTR(&pin_PB22) },


    // GROVE on SERCOM0
    { MP_ROM_QSTR(MP_QSTR_GROVE_SCL), MP_ROM_PTR(&pin_PA09) },
    { MP_ROM_QSTR(MP_QSTR_GROVE_SDA), MP_ROM_PTR(&pin_PA08) },
    { MP_ROM_QSTR(MP_QSTR_GROVE_RX), MP_ROM_PTR(&pin_PA09) },
    { MP_ROM_QSTR(MP_QSTR_GROVE_TX), MP_ROM_PTR(&pin_PA08) },
    { MP_ROM_QSTR(MP_QSTR_GROVE_D1), MP_ROM_PTR(&pin_PA09) },
    { MP_ROM_QSTR(MP_QSTR_GROVE_D0), MP_ROM_PTR(&pin_PA08) },
    { MP_ROM_QSTR(MP_QSTR_GROVE_A1), MP_ROM_PTR(&pin_PA09) },
    { MP_ROM_QSTR(MP_QSTR_GROVE_A0), MP_ROM_PTR(&pin_PA08) },

    // UART on SERCOM0
    { MP_ROM_QSTR(MP_QSTR_UART_TX), MP_ROM_PTR(&pin_PA04) },
    { MP_ROM_QSTR(MP_QSTR_UART_RX), MP_ROM_PTR(&pin_PA05) },
    { MP_ROM_QSTR(MP_QSTR_UART_CTS), MP_ROM_PTR(&pin_PA06) },
    { MP_ROM_QSTR(MP_QSTR_UART_RTS), MP_ROM_PTR(&pin_PA07) },

    // UART on SERCOM1 (Raspberry Pi)
    { MP_ROM_QSTR(MP_QSTR_TX1), MP_ROM_PTR(&pin_PA16) },
    { MP_ROM_QSTR(MP_QSTR_RX1), MP_ROM_PTR(&pin_PA17) },
    { MP_ROM_QSTR(MP_QSTR_PI_TX), MP_ROM_PTR(&pin_PA16) },
    { MP_ROM_QSTR(MP_QSTR_PI_RX), MP_ROM_PTR(&pin_PA17) },

    // I2C on SERCOM1 (External Connector)
    { MP_ROM_QSTR(MP_QSTR_SDA1), MP_ROM_PTR(&pin_PA00) },
    { MP_ROM_QSTR(MP_QSTR_SCL1), MP_ROM_PTR(&pin_PA01) },

    // SPI Flash on SERCOM2
    { MP_ROM_QSTR(MP_QSTR_FLASH_SCK), MP_ROM_PTR(&pin_PA13) },
    { MP_ROM_QSTR(MP_QSTR_FLASH_MISO), MP_ROM_PTR(&pin_PA14) },
    { MP_ROM_QSTR(MP_QSTR_FLASH_MOSI), MP_ROM_PTR(&pin_PA12) },
    { MP_ROM_QSTR(MP_QSTR_FLASH_CS), MP_ROM_PTR(&pin_PA15) },

    // I2C on SERCOM3 (RPi & Internal)
    { MP_ROM_QSTR(MP_QSTR_SDA), MP_ROM_PTR(&pin_PA22) },
    { MP_ROM_QSTR(MP_QSTR_SCL), MP_ROM_PTR(&pin_PA23) },
    { MP_ROM_QSTR(MP_QSTR_PI_SDA), MP_ROM_PTR(&pin_PA22) },
    { MP_ROM_QSTR(MP_QSTR_PI_SCL), MP_ROM_PTR(&pin_PA23) },

    // SPI on SERCOM4
    { MP_ROM_QSTR(MP_QSTR_MOSI), MP_ROM_PTR(&pin_PB08) },
    { MP_ROM_QSTR(MP_QSTR_SCK), MP_ROM_PTR(&pin_PB09) },
    { MP_ROM_QSTR(MP_QSTR_SS), MP_ROM_PTR(&pin_PB10) },
    { MP_ROM_QSTR(MP_QSTR_MISO), MP_ROM_PTR(&pin_PB11) },

    // GPS on SERCOM5
    { MP_ROM_QSTR(MP_QSTR_GPS_TX), MP_ROM_PTR(&pin_PB02) },
    { MP_ROM_QSTR(MP_QSTR_GPS_RX), MP_ROM_PTR(&pin_PB03) },

    // Raspberry Pi
    { MP_ROM_QSTR(MP_QSTR_PI_GP25), MP_ROM_PTR(&pin_PA30) },
    { MP_ROM_QSTR(MP_QSTR_SWCLK), MP_ROM_PTR(&pin_PA30) },
    { MP_ROM_QSTR(MP_QSTR_PI_GP24), MP_ROM_PTR(&pin_PA31) },
    { MP_ROM_QSTR(MP_QSTR_SWDIO), MP_ROM_PTR(&pin_PA31) },

    { MP_ROM_QSTR(MP_QSTR_I2C), MP_ROM_PTR(&board_i2c_obj) },
    { MP_ROM_QSTR(MP_QSTR_SPI), MP_ROM_PTR(&board_spi_obj) },
    // { MP_ROM_QSTR(MP_QSTR_UART), MP_ROM_PTR(&board_uart_obj) },
};
MP_DEFINE_CONST_DICT(board_module_globals, board_module_globals_table);
