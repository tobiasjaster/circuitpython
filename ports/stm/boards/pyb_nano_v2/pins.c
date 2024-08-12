// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/board/__init__.h"

static const mp_rom_map_elem_t board_module_globals_table[] = {
    CIRCUITPYTHON_BOARD_DICT_STANDARD_ITEMS

    { MP_ROM_QSTR(MP_QSTR_Y10), MP_ROM_PTR(&pin_PA10) },
    { MP_ROM_QSTR(MP_QSTR_Y9), MP_ROM_PTR(&pin_PA13) },
    { MP_ROM_QSTR(MP_QSTR_Y8), MP_ROM_PTR(&pin_PA14) },
    { MP_ROM_QSTR(MP_QSTR_Y7), MP_ROM_PTR(&pin_PA15) },
    { MP_ROM_QSTR(MP_QSTR_Y6), MP_ROM_PTR(&pin_PB03) },
    { MP_ROM_QSTR(MP_QSTR_Y5), MP_ROM_PTR(&pin_PB04) },
    { MP_ROM_QSTR(MP_QSTR_Y4), MP_ROM_PTR(&pin_PB05) },
    { MP_ROM_QSTR(MP_QSTR_Y3), MP_ROM_PTR(&pin_PB06) },
    { MP_ROM_QSTR(MP_QSTR_Y2), MP_ROM_PTR(&pin_PB07) },
    { MP_ROM_QSTR(MP_QSTR_Y1), MP_ROM_PTR(&pin_PB08) },
    { MP_ROM_QSTR(MP_QSTR_Y0), MP_ROM_PTR(&pin_PB09) },

    { MP_ROM_QSTR(MP_QSTR_X15), MP_ROM_PTR(&pin_PA08) },
    { MP_ROM_QSTR(MP_QSTR_X14), MP_ROM_PTR(&pin_PB15) },
    { MP_ROM_QSTR(MP_QSTR_X13), MP_ROM_PTR(&pin_PB14) },
    { MP_ROM_QSTR(MP_QSTR_X12), MP_ROM_PTR(&pin_PB13) },
    { MP_ROM_QSTR(MP_QSTR_X11), MP_ROM_PTR(&pin_PB12) },
    { MP_ROM_QSTR(MP_QSTR_X10), MP_ROM_PTR(&pin_PB10) },
    { MP_ROM_QSTR(MP_QSTR_X9), MP_ROM_PTR(&pin_PB01) },
    { MP_ROM_QSTR(MP_QSTR_X8), MP_ROM_PTR(&pin_PB00) },
    { MP_ROM_QSTR(MP_QSTR_X7), MP_ROM_PTR(&pin_PA07) },
    { MP_ROM_QSTR(MP_QSTR_X6), MP_ROM_PTR(&pin_PA06) },
    { MP_ROM_QSTR(MP_QSTR_X5), MP_ROM_PTR(&pin_PA05) },
    { MP_ROM_QSTR(MP_QSTR_X4), MP_ROM_PTR(&pin_PA04) },
    { MP_ROM_QSTR(MP_QSTR_X3), MP_ROM_PTR(&pin_PA03) },
    { MP_ROM_QSTR(MP_QSTR_X2), MP_ROM_PTR(&pin_PA02) },
    { MP_ROM_QSTR(MP_QSTR_X1), MP_ROM_PTR(&pin_PA01) },
    { MP_ROM_QSTR(MP_QSTR_X0), MP_ROM_PTR(&pin_PA00) },

    { MP_ROM_QSTR(MP_QSTR_SDA1), MP_ROM_PTR(&pin_PB09) },
    { MP_ROM_QSTR(MP_QSTR_SCL1), MP_ROM_PTR(&pin_PB08) },
    { MP_ROM_QSTR(MP_QSTR_SDA2), MP_ROM_PTR(&pin_PB03) },
    { MP_ROM_QSTR(MP_QSTR_SCL2), MP_ROM_PTR(&pin_PB10) },
    { MP_ROM_QSTR(MP_QSTR_SDA3), MP_ROM_PTR(&pin_PB04) },
    { MP_ROM_QSTR(MP_QSTR_SCL3), MP_ROM_PTR(&pin_PA08) },

    { MP_ROM_QSTR(MP_QSTR_SCK1), MP_ROM_PTR(&pin_PA05) },
    { MP_ROM_QSTR(MP_QSTR_MISO1), MP_ROM_PTR(&pin_PA06) },
    { MP_ROM_QSTR(MP_QSTR_MOSI1), MP_ROM_PTR(&pin_PA07) },
    { MP_ROM_QSTR(MP_QSTR_SCK2), MP_ROM_PTR(&pin_PB13) },
    { MP_ROM_QSTR(MP_QSTR_MISO2), MP_ROM_PTR(&pin_PB14) },
    { MP_ROM_QSTR(MP_QSTR_MOSI2), MP_ROM_PTR(&pin_PB15) },

    { MP_ROM_QSTR(MP_QSTR_TX1), MP_ROM_PTR(&pin_PB07) },
    { MP_ROM_QSTR(MP_QSTR_RX1), MP_ROM_PTR(&pin_PB06) },
    { MP_ROM_QSTR(MP_QSTR_TX2), MP_ROM_PTR(&pin_PA02) },
    { MP_ROM_QSTR(MP_QSTR_RX2), MP_ROM_PTR(&pin_PA03) },

    { MP_ROM_QSTR(MP_QSTR_LED_RED), MP_ROM_PTR(&pin_PA00) },
    { MP_ROM_QSTR(MP_QSTR_LED_GREEN), MP_ROM_PTR(&pin_PA01) },
    { MP_ROM_QSTR(MP_QSTR_LED_YELLOW), MP_ROM_PTR(&pin_PA02) },
    { MP_ROM_QSTR(MP_QSTR_LED_BLUE), MP_ROM_PTR(&pin_PA03) },

    { MP_ROM_QSTR(MP_QSTR_SW), MP_ROM_PTR(&pin_PC13) },
};
MP_DEFINE_CONST_DICT(board_module_globals, board_module_globals_table);
