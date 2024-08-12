// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/board/__init__.h"

static const mp_rom_map_elem_t board_module_globals_table[] = {
    CIRCUITPYTHON_BOARD_DICT_STANDARD_ITEMS

    { MP_ROM_QSTR(MP_QSTR_P0), MP_ROM_PTR(&pin_P0_02) }, // RING0
    { MP_ROM_QSTR(MP_QSTR_P1), MP_ROM_PTR(&pin_P0_03) }, // RING1
    { MP_ROM_QSTR(MP_QSTR_P2), MP_ROM_PTR(&pin_P0_04) }, // RING2
    { MP_ROM_QSTR(MP_QSTR_P3), MP_ROM_PTR(&pin_P0_31) }, // COLR3
    { MP_ROM_QSTR(MP_QSTR_P4), MP_ROM_PTR(&pin_P0_28) }, // COLR1
    { MP_ROM_QSTR(MP_QSTR_BTN_A), MP_ROM_PTR(&pin_P0_14) },
    { MP_ROM_QSTR(MP_QSTR_P5), MP_ROM_PTR(&pin_P0_14) }, // BTN A
    { MP_ROM_QSTR(MP_QSTR_P6), MP_ROM_PTR(&pin_P1_05) }, // COLR4
    { MP_ROM_QSTR(MP_QSTR_P7), MP_ROM_PTR(&pin_P0_11) }, // COLR2
    { MP_ROM_QSTR(MP_QSTR_P8), MP_ROM_PTR(&pin_P0_10) }, // GPIO1
    { MP_ROM_QSTR(MP_QSTR_P9), MP_ROM_PTR(&pin_P0_09) }, // GPIO2
    { MP_ROM_QSTR(MP_QSTR_P10), MP_ROM_PTR(&pin_P0_30) }, // COLR5
    { MP_ROM_QSTR(MP_QSTR_BTN_B), MP_ROM_PTR(&pin_P0_23) },
    { MP_ROM_QSTR(MP_QSTR_P11), MP_ROM_PTR(&pin_P0_23) }, // BTN B
    { MP_ROM_QSTR(MP_QSTR_P12), MP_ROM_PTR(&pin_P0_12) }, // GPIO4
    { MP_ROM_QSTR(MP_QSTR_P13), MP_ROM_PTR(&pin_P0_17) }, // SPI_EXT_SCK
    { MP_ROM_QSTR(MP_QSTR_SCK), MP_ROM_PTR(&pin_P0_17) }, // SPI_EXT_SCK
    { MP_ROM_QSTR(MP_QSTR_P14), MP_ROM_PTR(&pin_P0_01) }, // SPI_EXT_MISO
    { MP_ROM_QSTR(MP_QSTR_MISO), MP_ROM_PTR(&pin_P0_01) }, // SPI_EXT_MISO
    { MP_ROM_QSTR(MP_QSTR_P15), MP_ROM_PTR(&pin_P0_13) }, // SPI_EXT_MOSI
    { MP_ROM_QSTR(MP_QSTR_MOSI), MP_ROM_PTR(&pin_P0_13) }, // SPI_EXT_MOSI
    { MP_ROM_QSTR(MP_QSTR_P16), MP_ROM_PTR(&pin_P0_05) }, // GPIO3

    { MP_ROM_QSTR(MP_QSTR_P19), MP_ROM_PTR(&pin_P0_08) },
    { MP_ROM_QSTR(MP_QSTR_SCL), MP_ROM_PTR(&pin_P0_08) },
    { MP_ROM_QSTR(MP_QSTR_P20), MP_ROM_PTR(&pin_P0_16) },
    { MP_ROM_QSTR(MP_QSTR_SDA), MP_ROM_PTR(&pin_P0_16) },

    // Internal I2C
    { MP_ROM_QSTR(MP_QSTR_INTERNAL_SCL), MP_ROM_PTR(&pin_P0_26) },
    { MP_ROM_QSTR(MP_QSTR_INTERNAL_SDA), MP_ROM_PTR(&pin_P1_00) },
    { MP_ROM_QSTR(MP_QSTR_INTERNAL_INTERRUPT), MP_ROM_PTR(&pin_P0_25) },

    { MP_ROM_QSTR(MP_QSTR_MICROPHONE_ENABLE), MP_ROM_PTR(&pin_P0_20) },
    { MP_ROM_QSTR(MP_QSTR_MICROPHONE), MP_ROM_PTR(&pin_P0_05) },
    { MP_ROM_QSTR(MP_QSTR_SPEAKER), MP_ROM_PTR(&pin_P0_00) },

    { MP_ROM_QSTR(MP_QSTR_RX), MP_ROM_PTR(&pin_P0_06) },
    { MP_ROM_QSTR(MP_QSTR_TX), MP_ROM_PTR(&pin_P1_08) },

    { MP_ROM_QSTR(MP_QSTR_LOGO), MP_ROM_PTR(&pin_P1_04) },

    { MP_ROM_QSTR(MP_QSTR_COL1), MP_ROM_PTR(&pin_P0_28) },
    { MP_ROM_QSTR(MP_QSTR_COL2), MP_ROM_PTR(&pin_P0_11) },
    { MP_ROM_QSTR(MP_QSTR_COL3), MP_ROM_PTR(&pin_P0_31) },
    { MP_ROM_QSTR(MP_QSTR_COL4), MP_ROM_PTR(&pin_P1_05) },
    { MP_ROM_QSTR(MP_QSTR_COL5), MP_ROM_PTR(&pin_P0_30) },

    { MP_ROM_QSTR(MP_QSTR_ROW1), MP_ROM_PTR(&pin_P0_21) },
    { MP_ROM_QSTR(MP_QSTR_ROW2), MP_ROM_PTR(&pin_P0_22) },
    { MP_ROM_QSTR(MP_QSTR_ROW3), MP_ROM_PTR(&pin_P0_15) },
    { MP_ROM_QSTR(MP_QSTR_ROW4), MP_ROM_PTR(&pin_P0_24) },
    { MP_ROM_QSTR(MP_QSTR_ROW5), MP_ROM_PTR(&pin_P0_19) },
};

MP_DEFINE_CONST_DICT(board_module_globals, board_module_globals_table);
