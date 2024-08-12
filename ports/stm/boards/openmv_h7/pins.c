// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/board/__init__.h"

static const mp_rom_map_elem_t board_module_globals_table[] = {
    CIRCUITPYTHON_BOARD_DICT_STANDARD_ITEMS

    { MP_ROM_QSTR(MP_QSTR_P0), MP_ROM_PTR(&pin_PB15) },
    { MP_ROM_QSTR(MP_QSTR_P1), MP_ROM_PTR(&pin_PB14) },
    { MP_ROM_QSTR(MP_QSTR_P2), MP_ROM_PTR(&pin_PB13) },
    { MP_ROM_QSTR(MP_QSTR_P3), MP_ROM_PTR(&pin_PB12) },
    { MP_ROM_QSTR(MP_QSTR_P4), MP_ROM_PTR(&pin_PB10) },
    { MP_ROM_QSTR(MP_QSTR_P5), MP_ROM_PTR(&pin_PB11) },
    { MP_ROM_QSTR(MP_QSTR_P6), MP_ROM_PTR(&pin_PA05) },
    { MP_ROM_QSTR(MP_QSTR_P7), MP_ROM_PTR(&pin_PD12) },
    { MP_ROM_QSTR(MP_QSTR_P8), MP_ROM_PTR(&pin_PD13) },
    { MP_ROM_QSTR(MP_QSTR_P9), MP_ROM_PTR(&pin_PD14) },
    { MP_ROM_QSTR(MP_QSTR_LED1), MP_ROM_PTR(&pin_PC00) },
    { MP_ROM_QSTR(MP_QSTR_LED2), MP_ROM_PTR(&pin_PC01) },
    { MP_ROM_QSTR(MP_QSTR_LED3), MP_ROM_PTR(&pin_PC02) },
    { MP_ROM_QSTR(MP_QSTR_LED4), MP_ROM_PTR(&pin_PE02) },
    { MP_ROM_QSTR(MP_QSTR_LED_RED), MP_ROM_PTR(&pin_PC00) },
    { MP_ROM_QSTR(MP_QSTR_LED_GREEN), MP_ROM_PTR(&pin_PC01) },
    { MP_ROM_QSTR(MP_QSTR_LED_BLUE), MP_ROM_PTR(&pin_PC02) },
    { MP_ROM_QSTR(MP_QSTR_LED_IR), MP_ROM_PTR(&pin_PE02) },
    { MP_ROM_QSTR(MP_QSTR_UART1_TX), MP_ROM_PTR(&pin_PB14) },
    { MP_ROM_QSTR(MP_QSTR_UART1_RX), MP_ROM_PTR(&pin_PB15) },
    { MP_ROM_QSTR(MP_QSTR_UART3_TX), MP_ROM_PTR(&pin_PB10) },
    { MP_ROM_QSTR(MP_QSTR_UART3_RX), MP_ROM_PTR(&pin_PB11) },
    { MP_ROM_QSTR(MP_QSTR_I2C2_SCL), MP_ROM_PTR(&pin_PB10) },
    { MP_ROM_QSTR(MP_QSTR_I2C2_SDA), MP_ROM_PTR(&pin_PB11) },
    { MP_ROM_QSTR(MP_QSTR_I2C4_SCL), MP_ROM_PTR(&pin_PD12) },
    { MP_ROM_QSTR(MP_QSTR_I2C4_SDA), MP_ROM_PTR(&pin_PD13) },
    { MP_ROM_QSTR(MP_QSTR_SPI2_NSS), MP_ROM_PTR(&pin_PB12) },
    { MP_ROM_QSTR(MP_QSTR_SPI2_SCK), MP_ROM_PTR(&pin_PB13) },
    { MP_ROM_QSTR(MP_QSTR_SPI2_MISO), MP_ROM_PTR(&pin_PB14) },
    { MP_ROM_QSTR(MP_QSTR_SPI2_MOSI), MP_ROM_PTR(&pin_PB15) },
};
MP_DEFINE_CONST_DICT(board_module_globals, board_module_globals_table);
