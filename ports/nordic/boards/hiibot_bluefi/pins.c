// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/board/__init__.h"

#include "supervisor/board.h"
#include "shared-module/displayio/__init__.h"

static const mp_rom_map_elem_t board_module_globals_table[] = {
    CIRCUITPYTHON_BOARD_DICT_STANDARD_ITEMS

    { MP_ROM_QSTR(MP_QSTR_P0), MP_ROM_PTR(&pin_P0_28) },
    { MP_ROM_QSTR(MP_QSTR_D0), MP_ROM_PTR(&pin_P0_28) },
    { MP_ROM_QSTR(MP_QSTR_A0), MP_ROM_PTR(&pin_P0_28) },
    { MP_ROM_QSTR(MP_QSTR_RX), MP_ROM_PTR(&pin_P0_28) },

    { MP_ROM_QSTR(MP_QSTR_P1), MP_ROM_PTR(&pin_P0_02) },
    { MP_ROM_QSTR(MP_QSTR_D1), MP_ROM_PTR(&pin_P0_02) },
    { MP_ROM_QSTR(MP_QSTR_A1), MP_ROM_PTR(&pin_P0_02) },
    { MP_ROM_QSTR(MP_QSTR_TX), MP_ROM_PTR(&pin_P0_02) },

    { MP_ROM_QSTR(MP_QSTR_P2), MP_ROM_PTR(&pin_P0_29) },
    { MP_ROM_QSTR(MP_QSTR_D2), MP_ROM_PTR(&pin_P0_29) },
    { MP_ROM_QSTR(MP_QSTR_A2), MP_ROM_PTR(&pin_P0_29) },

    { MP_ROM_QSTR(MP_QSTR_P3), MP_ROM_PTR(&pin_P0_30) },
    { MP_ROM_QSTR(MP_QSTR_D3), MP_ROM_PTR(&pin_P0_30) },
    { MP_ROM_QSTR(MP_QSTR_A3), MP_ROM_PTR(&pin_P0_30) },

    { MP_ROM_QSTR(MP_QSTR_P4), MP_ROM_PTR(&pin_P0_03) },
    { MP_ROM_QSTR(MP_QSTR_D4), MP_ROM_PTR(&pin_P0_03) },
    { MP_ROM_QSTR(MP_QSTR_A4), MP_ROM_PTR(&pin_P0_03) },

    { MP_ROM_QSTR(MP_QSTR_P5), MP_ROM_PTR(&pin_P1_07) },
    { MP_ROM_QSTR(MP_QSTR_D5), MP_ROM_PTR(&pin_P1_07) },
    { MP_ROM_QSTR(MP_QSTR_BUTTON_A), MP_ROM_PTR(&pin_P1_07) },

    { MP_ROM_QSTR(MP_QSTR_P6), MP_ROM_PTR(&pin_P0_08) },
    { MP_ROM_QSTR(MP_QSTR_D6), MP_ROM_PTR(&pin_P0_08) },

    { MP_ROM_QSTR(MP_QSTR_P7), MP_ROM_PTR(&pin_P0_25) },
    { MP_ROM_QSTR(MP_QSTR_D7), MP_ROM_PTR(&pin_P0_25) },

    { MP_ROM_QSTR(MP_QSTR_P8), MP_ROM_PTR(&pin_P0_23) },
    { MP_ROM_QSTR(MP_QSTR_D8), MP_ROM_PTR(&pin_P0_23) },

    { MP_ROM_QSTR(MP_QSTR_P9), MP_ROM_PTR(&pin_P0_21) },
    { MP_ROM_QSTR(MP_QSTR_D9), MP_ROM_PTR(&pin_P0_21) },

    { MP_ROM_QSTR(MP_QSTR_P10), MP_ROM_PTR(&pin_P0_19) },
    { MP_ROM_QSTR(MP_QSTR_D10), MP_ROM_PTR(&pin_P0_19) },

    { MP_ROM_QSTR(MP_QSTR_P11), MP_ROM_PTR(&pin_P1_09) },
    { MP_ROM_QSTR(MP_QSTR_D11), MP_ROM_PTR(&pin_P1_09) },
    { MP_ROM_QSTR(MP_QSTR_BUTTON_B), MP_ROM_PTR(&pin_P1_09) },

    { MP_ROM_QSTR(MP_QSTR_P12), MP_ROM_PTR(&pin_P0_16) },
    { MP_ROM_QSTR(MP_QSTR_D12), MP_ROM_PTR(&pin_P0_16) },

    { MP_ROM_QSTR(MP_QSTR_P13), MP_ROM_PTR(&pin_P0_06) },
    { MP_ROM_QSTR(MP_QSTR_D13), MP_ROM_PTR(&pin_P0_06) },
    { MP_ROM_QSTR(MP_QSTR_SCK), MP_ROM_PTR(&pin_P0_06) },

    { MP_ROM_QSTR(MP_QSTR_P14), MP_ROM_PTR(&pin_P0_04) },
    { MP_ROM_QSTR(MP_QSTR_D14), MP_ROM_PTR(&pin_P0_04) },
    { MP_ROM_QSTR(MP_QSTR_A5), MP_ROM_PTR(&pin_P0_04) },
    { MP_ROM_QSTR(MP_QSTR_MISO), MP_ROM_PTR(&pin_P0_04) },

    { MP_ROM_QSTR(MP_QSTR_P15), MP_ROM_PTR(&pin_P0_26) },
    { MP_ROM_QSTR(MP_QSTR_D15), MP_ROM_PTR(&pin_P0_26) },
    { MP_ROM_QSTR(MP_QSTR_MOSI), MP_ROM_PTR(&pin_P0_26) },

    { MP_ROM_QSTR(MP_QSTR_P16), MP_ROM_PTR(&pin_P0_01) },
    { MP_ROM_QSTR(MP_QSTR_D16), MP_ROM_PTR(&pin_P0_01) },

    { MP_ROM_QSTR(MP_QSTR_P17), MP_ROM_PTR(&pin_P1_12) },
    { MP_ROM_QSTR(MP_QSTR_D17), MP_ROM_PTR(&pin_P1_12) },
    { MP_ROM_QSTR(MP_QSTR_LED), MP_ROM_PTR(&pin_P1_12) },
    { MP_ROM_QSTR(MP_QSTR_REDLED), MP_ROM_PTR(&pin_P1_12) },

    { MP_ROM_QSTR(MP_QSTR_P18), MP_ROM_PTR(&pin_P1_10) },
    { MP_ROM_QSTR(MP_QSTR_D18), MP_ROM_PTR(&pin_P1_10) },
    { MP_ROM_QSTR(MP_QSTR_NEOPIXEL), MP_ROM_PTR(&pin_P1_10) },

    { MP_ROM_QSTR(MP_QSTR_P19), MP_ROM_PTR(&pin_P0_00) },
    { MP_ROM_QSTR(MP_QSTR_D19), MP_ROM_PTR(&pin_P0_00) },
    { MP_ROM_QSTR(MP_QSTR_SCL), MP_ROM_PTR(&pin_P0_00) },

    { MP_ROM_QSTR(MP_QSTR_P20), MP_ROM_PTR(&pin_P0_31) },
    { MP_ROM_QSTR(MP_QSTR_D20), MP_ROM_PTR(&pin_P0_31) },
    { MP_ROM_QSTR(MP_QSTR_A6), MP_ROM_PTR(&pin_P0_31) },
    { MP_ROM_QSTR(MP_QSTR_SDA), MP_ROM_PTR(&pin_P0_31) },

    { MP_ROM_QSTR(MP_QSTR_P21), MP_ROM_PTR(&pin_P0_09) },
    { MP_ROM_QSTR(MP_QSTR_D21), MP_ROM_PTR(&pin_P0_09) },
    { MP_ROM_QSTR(MP_QSTR_MICROPHONE_CLOCK), MP_ROM_PTR(&pin_P0_09) },

    { MP_ROM_QSTR(MP_QSTR_P22), MP_ROM_PTR(&pin_P0_10) },
    { MP_ROM_QSTR(MP_QSTR_D22), MP_ROM_PTR(&pin_P0_10) },
    { MP_ROM_QSTR(MP_QSTR_MICROPHONE_DATA), MP_ROM_PTR(&pin_P0_10) },

    { MP_ROM_QSTR(MP_QSTR_P23), MP_ROM_PTR(&pin_P0_07) },
    { MP_ROM_QSTR(MP_QSTR_D23), MP_ROM_PTR(&pin_P0_07) },
    { MP_ROM_QSTR(MP_QSTR_TFT_SCK), MP_ROM_PTR(&pin_P0_07) },

    { MP_ROM_QSTR(MP_QSTR_P24), MP_ROM_PTR(&pin_P1_08) },
    { MP_ROM_QSTR(MP_QSTR_D24), MP_ROM_PTR(&pin_P1_08) },
    { MP_ROM_QSTR(MP_QSTR_TFT_MOSI), MP_ROM_PTR(&pin_P1_08) },

    { MP_ROM_QSTR(MP_QSTR_P25), MP_ROM_PTR(&pin_P0_05) },
    { MP_ROM_QSTR(MP_QSTR_D25), MP_ROM_PTR(&pin_P0_05) },
    { MP_ROM_QSTR(MP_QSTR_TFT_CS), MP_ROM_PTR(&pin_P0_05) },

    { MP_ROM_QSTR(MP_QSTR_P26), MP_ROM_PTR(&pin_P0_27) },
    { MP_ROM_QSTR(MP_QSTR_D26), MP_ROM_PTR(&pin_P0_27) },
    { MP_ROM_QSTR(MP_QSTR_TFT_DC), MP_ROM_PTR(&pin_P0_27) },

    { MP_ROM_QSTR(MP_QSTR_P27), MP_ROM_PTR(&pin_P1_13) },
    { MP_ROM_QSTR(MP_QSTR_D27), MP_ROM_PTR(&pin_P1_13) },
    // { MP_ROM_QSTR(MP_QSTR_TFT_RESET), MP_ROM_PTR(&pin_P1_13) },
    { MP_ROM_QSTR(MP_QSTR_TFT_BACKLIGHT), MP_ROM_PTR(&pin_P1_13) },

    // P28~P33/D28~D33 connected into QSPI FlashROM (W25Q16JV_IQ)

    { MP_ROM_QSTR(MP_QSTR_P34), MP_ROM_PTR(&pin_P0_22) },
    { MP_ROM_QSTR(MP_QSTR_D34), MP_ROM_PTR(&pin_P0_22) },
    { MP_ROM_QSTR(MP_QSTR_WIFI_SCK), MP_ROM_PTR(&pin_P0_22) },

    { MP_ROM_QSTR(MP_QSTR_P35), MP_ROM_PTR(&pin_P0_17) },
    { MP_ROM_QSTR(MP_QSTR_D35), MP_ROM_PTR(&pin_P0_17) },
    { MP_ROM_QSTR(MP_QSTR_WIFI_MISO), MP_ROM_PTR(&pin_P0_17) },

    { MP_ROM_QSTR(MP_QSTR_P36), MP_ROM_PTR(&pin_P0_20) },
    { MP_ROM_QSTR(MP_QSTR_D36), MP_ROM_PTR(&pin_P0_20) },
    { MP_ROM_QSTR(MP_QSTR_WIFI_MOSI), MP_ROM_PTR(&pin_P0_20) },

    { MP_ROM_QSTR(MP_QSTR_P37), MP_ROM_PTR(&pin_P0_15) },
    { MP_ROM_QSTR(MP_QSTR_D37), MP_ROM_PTR(&pin_P0_15) },
    { MP_ROM_QSTR(MP_QSTR_WIFI_BUSY), MP_ROM_PTR(&pin_P0_15) },

    { MP_ROM_QSTR(MP_QSTR_P38), MP_ROM_PTR(&pin_P0_24) },
    { MP_ROM_QSTR(MP_QSTR_D38), MP_ROM_PTR(&pin_P0_24) },
    { MP_ROM_QSTR(MP_QSTR_WIFI_CS), MP_ROM_PTR(&pin_P0_24) },

    { MP_ROM_QSTR(MP_QSTR_P39), MP_ROM_PTR(&pin_P1_00) },
    { MP_ROM_QSTR(MP_QSTR_D39), MP_ROM_PTR(&pin_P1_00) },
    { MP_ROM_QSTR(MP_QSTR_WIFI_RESET), MP_ROM_PTR(&pin_P1_00) },

    { MP_ROM_QSTR(MP_QSTR_P40), MP_ROM_PTR(&pin_P0_13) },
    { MP_ROM_QSTR(MP_QSTR_D40), MP_ROM_PTR(&pin_P0_13) },
    { MP_ROM_QSTR(MP_QSTR_WIFI_PWR), MP_ROM_PTR(&pin_P0_13) },

    { MP_ROM_QSTR(MP_QSTR_P41), MP_ROM_PTR(&pin_P0_11) },
    { MP_ROM_QSTR(MP_QSTR_D41), MP_ROM_PTR(&pin_P0_11) },
    { MP_ROM_QSTR(MP_QSTR_SENSORS_SCL), MP_ROM_PTR(&pin_P0_11) },

    { MP_ROM_QSTR(MP_QSTR_P42), MP_ROM_PTR(&pin_P0_12) },
    { MP_ROM_QSTR(MP_QSTR_D42), MP_ROM_PTR(&pin_P0_12) },
    { MP_ROM_QSTR(MP_QSTR_SENSORS_SDA), MP_ROM_PTR(&pin_P0_12) },

    { MP_ROM_QSTR(MP_QSTR_P43), MP_ROM_PTR(&pin_P0_14) },
    { MP_ROM_QSTR(MP_QSTR_D43), MP_ROM_PTR(&pin_P0_14) },
    { MP_ROM_QSTR(MP_QSTR_IMU_IRQ), MP_ROM_PTR(&pin_P0_14) },
    { MP_ROM_QSTR(MP_QSTR_ACCELEROMETER_INTERRUPT), MP_ROM_PTR(&pin_P0_14) },

    { MP_ROM_QSTR(MP_QSTR_P44), MP_ROM_PTR(&pin_P1_14) },
    { MP_ROM_QSTR(MP_QSTR_D44), MP_ROM_PTR(&pin_P1_14) },
    { MP_ROM_QSTR(MP_QSTR_WHITELED), MP_ROM_PTR(&pin_P1_14) },

    { MP_ROM_QSTR(MP_QSTR_P45), MP_ROM_PTR(&pin_P1_11) },
    { MP_ROM_QSTR(MP_QSTR_D45), MP_ROM_PTR(&pin_P1_11) },
    { MP_ROM_QSTR(MP_QSTR_SPEAKER_ENABLE), MP_ROM_PTR(&pin_P1_11) },

    { MP_ROM_QSTR(MP_QSTR_P46), MP_ROM_PTR(&pin_P1_15) },
    { MP_ROM_QSTR(MP_QSTR_D46), MP_ROM_PTR(&pin_P1_15) },
    { MP_ROM_QSTR(MP_QSTR_SPEAKER), MP_ROM_PTR(&pin_P1_15) },
    { MP_ROM_QSTR(MP_QSTR_AUDIO), MP_ROM_PTR(&pin_P1_15) },

    { MP_ROM_QSTR(MP_QSTR_I2C), MP_ROM_PTR(&board_i2c_obj) },
    { MP_ROM_QSTR(MP_QSTR_STEMMA_I2C), MP_ROM_PTR(&board_i2c_obj) },
    { MP_ROM_QSTR(MP_QSTR_SPI), MP_ROM_PTR(&board_spi_obj) },
    { MP_ROM_QSTR(MP_QSTR_UART), MP_ROM_PTR(&board_uart_obj) },

    { MP_ROM_QSTR(MP_QSTR_DISPLAY), MP_ROM_PTR(&displays[0].display)}
};

MP_DEFINE_CONST_DICT(board_module_globals, board_module_globals_table);
