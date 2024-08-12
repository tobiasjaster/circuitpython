// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2018 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "py/objtuple.h"
#include "shared-bindings/board/__init__.h"

static const mp_rom_obj_tuple_t sdio_data_tuple = {
    {&mp_type_tuple},
    4,
    {
        MP_ROM_PTR(&pin_PB18),
        MP_ROM_PTR(&pin_PB19),
        MP_ROM_PTR(&pin_PB20),
        MP_ROM_PTR(&pin_PB21),
    }
};


// This mapping only includes functional names because pins broken
// out on connectors are labeled with their MCU name available from
// microcontroller.pin.
static const mp_rom_map_elem_t board_module_globals_table[] = {
    CIRCUITPYTHON_BOARD_DICT_STANDARD_ITEMS

    { MP_OBJ_NEW_QSTR(MP_QSTR_AREF),  MP_ROM_PTR(&pin_PA03) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_A0),  MP_ROM_PTR(&pin_PA02) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_A1),  MP_ROM_PTR(&pin_PA05) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_A2),  MP_ROM_PTR(&pin_PB03) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_A3),  MP_ROM_PTR(&pin_PC00) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_A4),  MP_ROM_PTR(&pin_PC01) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_A5),  MP_ROM_PTR(&pin_PC02) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_A6),  MP_ROM_PTR(&pin_PC03) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_A7),  MP_ROM_PTR(&pin_PB04) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_A8),  MP_ROM_PTR(&pin_PB05) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_A9),  MP_ROM_PTR(&pin_PB06) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_A10),  MP_ROM_PTR(&pin_PB07) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_A11),  MP_ROM_PTR(&pin_PB08) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_A12),  MP_ROM_PTR(&pin_PB09) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_A13),  MP_ROM_PTR(&pin_PA04) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_A14),  MP_ROM_PTR(&pin_PA06) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_A15),  MP_ROM_PTR(&pin_PA07) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D0),  MP_ROM_PTR(&pin_PB25) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_RX),  MP_ROM_PTR(&pin_PB25) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D1),  MP_ROM_PTR(&pin_PB24) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_TX),  MP_ROM_PTR(&pin_PB24) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D2),  MP_ROM_PTR(&pin_PC18) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D3),  MP_ROM_PTR(&pin_PC19) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D4),  MP_ROM_PTR(&pin_PC20) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D5),  MP_ROM_PTR(&pin_PC21) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D6),  MP_ROM_PTR(&pin_PD20) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D7),  MP_ROM_PTR(&pin_PD21) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D8),  MP_ROM_PTR(&pin_PB18) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D9),  MP_ROM_PTR(&pin_PB02) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D10),  MP_ROM_PTR(&pin_PB22) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D11),  MP_ROM_PTR(&pin_PB23) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D12),  MP_ROM_PTR(&pin_PB00) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_LED), MP_ROM_PTR(&pin_PB01) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D13), MP_ROM_PTR(&pin_PB01) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D14), MP_ROM_PTR(&pin_PB16) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_TX3), MP_ROM_PTR(&pin_PB16) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D15), MP_ROM_PTR(&pin_PB17) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_RX3), MP_ROM_PTR(&pin_PB17) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D16), MP_ROM_PTR(&pin_PC22) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_TX2), MP_ROM_PTR(&pin_PC22) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D17), MP_ROM_PTR(&pin_PC23) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_RX2), MP_ROM_PTR(&pin_PC23) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D18), MP_ROM_PTR(&pin_PB12) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_TX1), MP_ROM_PTR(&pin_PB12) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D19), MP_ROM_PTR(&pin_PB13) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_RX1), MP_ROM_PTR(&pin_PB13) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D20), MP_ROM_PTR(&pin_PB20) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SDA), MP_ROM_PTR(&pin_PB20) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D21), MP_ROM_PTR(&pin_PB21) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SCL), MP_ROM_PTR(&pin_PB21) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D22), MP_ROM_PTR(&pin_PD12) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D23), MP_ROM_PTR(&pin_PA15) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D24), MP_ROM_PTR(&pin_PC17) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SCL1), MP_ROM_PTR(&pin_PC17) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D25), MP_ROM_PTR(&pin_PC16) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SDA1), MP_ROM_PTR(&pin_PC16) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D26), MP_ROM_PTR(&pin_PA12) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PCC_DEN1), MP_ROM_PTR(&pin_PA12) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D27), MP_ROM_PTR(&pin_PA13) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PCC_DEN2), MP_ROM_PTR(&pin_PA13) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D28), MP_ROM_PTR(&pin_PA14) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PCC_CLK), MP_ROM_PTR(&pin_PA14) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D29), MP_ROM_PTR(&pin_PB19) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PCC_XCLK), MP_ROM_PTR(&pin_PB19) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D30), MP_ROM_PTR(&pin_PA23) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PCC_D7), MP_ROM_PTR(&pin_PA23) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D31), MP_ROM_PTR(&pin_PA22) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PCC_D6), MP_ROM_PTR(&pin_PA22) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D32), MP_ROM_PTR(&pin_PA21) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PCC_D5), MP_ROM_PTR(&pin_PA21) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D33), MP_ROM_PTR(&pin_PA20) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PCC_D4), MP_ROM_PTR(&pin_PA20) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D34), MP_ROM_PTR(&pin_PA19) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PCC_D3), MP_ROM_PTR(&pin_PA19) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D35), MP_ROM_PTR(&pin_PA18) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PCC_D2), MP_ROM_PTR(&pin_PA18) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D36), MP_ROM_PTR(&pin_PA17) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PCC_D1), MP_ROM_PTR(&pin_PA17) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D37), MP_ROM_PTR(&pin_PA16) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PCC_D0), MP_ROM_PTR(&pin_PA16) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D38), MP_ROM_PTR(&pin_PB15) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PCC_D9), MP_ROM_PTR(&pin_PB15) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D39), MP_ROM_PTR(&pin_PB14) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PCC_D8), MP_ROM_PTR(&pin_PB14) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D40), MP_ROM_PTR(&pin_PC13) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PCC_D11), MP_ROM_PTR(&pin_PC13) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D41), MP_ROM_PTR(&pin_PC12) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PCC_D10), MP_ROM_PTR(&pin_PC12) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D42), MP_ROM_PTR(&pin_PC15) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PCC_D13), MP_ROM_PTR(&pin_PC15) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D43), MP_ROM_PTR(&pin_PC14) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PCC_D12), MP_ROM_PTR(&pin_PC14) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D44), MP_ROM_PTR(&pin_PC11) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D45), MP_ROM_PTR(&pin_PC10) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D46), MP_ROM_PTR(&pin_PC06) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D47), MP_ROM_PTR(&pin_PC07) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D48), MP_ROM_PTR(&pin_PC04) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D49), MP_ROM_PTR(&pin_PC05) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D50), MP_ROM_PTR(&pin_PD11) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MISO), MP_ROM_PTR(&pin_PD11) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D51), MP_ROM_PTR(&pin_PD08) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MOSI), MP_ROM_PTR(&pin_PD08) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D52), MP_ROM_PTR(&pin_PD09) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SCK), MP_ROM_PTR(&pin_PD09) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_D53), MP_ROM_PTR(&pin_PD10) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SS), MP_ROM_PTR(&pin_PD10) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_SD_MOSI), MP_ROM_PTR(&pin_PB26) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SD_SCK), MP_ROM_PTR(&pin_PB27) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SD_CS), MP_ROM_PTR(&pin_PB28) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SD_MISO), MP_ROM_PTR(&pin_PB29) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_SD_CARD_DETECT), MP_ROM_PTR(&pin_PB31) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_NEOPIXEL), MP_ROM_PTR(&pin_PC24) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_LED_RX), MP_ROM_PTR(&pin_PC31) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_LED_TX), MP_ROM_PTR(&pin_PC30) },
    { MP_ROM_QSTR(MP_QSTR_I2C), MP_ROM_PTR(&board_i2c_obj) },
    { MP_ROM_QSTR(MP_QSTR_SPI), MP_ROM_PTR(&board_spi_obj) },
    { MP_ROM_QSTR(MP_QSTR_UART), MP_ROM_PTR(&board_uart_obj) },

    { MP_ROM_QSTR(MP_QSTR_SDIO_CLOCK), MP_ROM_PTR(&pin_PA21) },
    { MP_ROM_QSTR(MP_QSTR_SDIO_COMMAND), MP_ROM_PTR(&pin_PA20) },
    { MP_ROM_QSTR(MP_QSTR_SDIO_DATA), MP_ROM_PTR(&sdio_data_tuple) },
};
MP_DEFINE_CONST_DICT(board_module_globals, board_module_globals_table);
