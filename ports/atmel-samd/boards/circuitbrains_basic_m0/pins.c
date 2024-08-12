// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Original work copyright (c) 2017 Scott Shawcroft for Adafruit Industries
// SPDX-FileCopyrightText: Modified work copyright (c) 2019 Kevin Neubauer for Null Byte Labs LLC
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/board/__init__.h"

// This mapping only includes functional names because pins broken
// out on connectors are labeled with their MCU name available from
// microcontroller.pin.
static const mp_rom_map_elem_t board_module_globals_table[] = {
    CIRCUITPYTHON_BOARD_DICT_STANDARD_ITEMS

    { MP_OBJ_NEW_QSTR(MP_QSTR_A0), MP_ROM_PTR(&pin_PA02) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_A1), MP_ROM_PTR(&pin_PA06) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_TX), MP_ROM_PTR(&pin_PA06) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_A2), MP_ROM_PTR(&pin_PA07) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_RX), MP_ROM_PTR(&pin_PA07) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_A3), MP_ROM_PTR(&pin_PA08) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D0), MP_ROM_PTR(&pin_PA00) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D1), MP_ROM_PTR(&pin_PA01) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D2), MP_ROM_PTR(&pin_PA28) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D3), MP_ROM_PTR(&pin_PA27) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D4), MP_ROM_PTR(&pin_PA23) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D5), MP_ROM_PTR(&pin_PA22) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D6), MP_ROM_PTR(&pin_PA15) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D7), MP_ROM_PTR(&pin_PA14) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_LED), MP_ROM_PTR(&pin_PA14) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_STATUS_LED), MP_ROM_PTR(&pin_PA14) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_SDA), MP_ROM_PTR(&pin_PA04) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SCL), MP_ROM_PTR(&pin_PA05) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_SCK), MP_ROM_PTR(&pin_PA11) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MOSI), MP_ROM_PTR(&pin_PA10) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MISO), MP_ROM_PTR(&pin_PA09) },

    { MP_ROM_QSTR(MP_QSTR_I2C), MP_ROM_PTR(&board_i2c_obj) },
    { MP_ROM_QSTR(MP_QSTR_SPI), MP_ROM_PTR(&board_spi_obj) },
    { MP_ROM_QSTR(MP_QSTR_UART), MP_ROM_PTR(&board_uart_obj) },
};
MP_DEFINE_CONST_DICT(board_module_globals, board_module_globals_table);
