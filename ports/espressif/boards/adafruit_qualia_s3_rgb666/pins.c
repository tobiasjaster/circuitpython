// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "py/objtuple.h"
#include "shared-bindings/board/__init__.h"

#define MP_DEFINE_BYTES_OBJ(obj_name, bin) mp_obj_str_t obj_name = {{&mp_type_bytes}, 0, sizeof(bin) - 1, (const byte *)bin}

static const char i2c_bus_init_sequence[] = {
    2, 3, 0x78, // set GPIO direction
    2, 2, 0, // disable all output inversion
    0, // trailing NUL for python bytes() representation
};
static MP_DEFINE_BYTES_OBJ(i2c_init_byte_obj, i2c_bus_init_sequence);

static const mp_rom_map_elem_t tft_io_expander_table[] = {
    { MP_ROM_QSTR(MP_QSTR_i2c_address), MP_ROM_INT(0x3f)},
    { MP_ROM_QSTR(MP_QSTR_gpio_address), MP_ROM_INT(1)},
    { MP_ROM_QSTR(MP_QSTR_gpio_data_len), MP_ROM_INT(1)},
    { MP_ROM_QSTR(MP_QSTR_gpio_data), MP_ROM_INT(0xFD)},
    { MP_ROM_QSTR(MP_QSTR_cs_bit), MP_ROM_INT(1)},
    { MP_ROM_QSTR(MP_QSTR_mosi_bit), MP_ROM_INT(7)},
    { MP_ROM_QSTR(MP_QSTR_clk_bit), MP_ROM_INT(0)},
    { MP_ROM_QSTR(MP_QSTR_reset_bit), MP_ROM_INT(2)},
    { MP_ROM_QSTR(MP_QSTR_i2c_init_sequence), &i2c_init_byte_obj},
};
MP_DEFINE_CONST_DICT(tft_io_expander_dict, tft_io_expander_table);

static const mp_rom_obj_tuple_t tft_r_pins = {
    {&mp_type_tuple},
    5,
    {
        MP_ROM_PTR(&pin_GPIO11),
        MP_ROM_PTR(&pin_GPIO10),
        MP_ROM_PTR(&pin_GPIO9),
        MP_ROM_PTR(&pin_GPIO46),
        MP_ROM_PTR(&pin_GPIO3),
    }
};

static const mp_rom_obj_tuple_t tft_g_pins = {
    {&mp_type_tuple},
    6,
    {
        MP_ROM_PTR(&pin_GPIO48),
        MP_ROM_PTR(&pin_GPIO47),
        MP_ROM_PTR(&pin_GPIO21),
        MP_ROM_PTR(&pin_GPIO14),
        MP_ROM_PTR(&pin_GPIO13),
        MP_ROM_PTR(&pin_GPIO12),
    }
};

static const mp_rom_obj_tuple_t tft_b_pins = {
    {&mp_type_tuple},
    5,
    {
        MP_ROM_PTR(&pin_GPIO40),
        MP_ROM_PTR(&pin_GPIO39),
        MP_ROM_PTR(&pin_GPIO38),
        MP_ROM_PTR(&pin_GPIO0),
        MP_ROM_PTR(&pin_GPIO45),
    }
};

static const mp_rom_map_elem_t tft_table[] = {
    { MP_ROM_QSTR(MP_QSTR_de), MP_ROM_PTR(&pin_GPIO2) },
    { MP_ROM_QSTR(MP_QSTR_vsync), MP_ROM_PTR(&pin_GPIO42) },
    { MP_ROM_QSTR(MP_QSTR_hsync), MP_ROM_PTR(&pin_GPIO41) },
    { MP_ROM_QSTR(MP_QSTR_dclk), MP_ROM_PTR(&pin_GPIO1) },
    { MP_ROM_QSTR(MP_QSTR_red), MP_ROM_PTR(&tft_r_pins) },
    { MP_ROM_QSTR(MP_QSTR_green), MP_ROM_PTR(&tft_g_pins) },
    { MP_ROM_QSTR(MP_QSTR_blue), MP_ROM_PTR(&tft_b_pins) },
};
MP_DEFINE_CONST_DICT(tft_dict, tft_table);

static const mp_rom_map_elem_t board_module_globals_table[] = {
    CIRCUITPYTHON_BOARD_DICT_STANDARD_ITEMS

    { MP_ROM_QSTR(MP_QSTR_TFT_PINS), MP_ROM_PTR(&tft_dict) },
    { MP_ROM_QSTR(MP_QSTR_TFT_IO_EXPANDER), MP_ROM_PTR(&tft_io_expander_dict) },

    { MP_ROM_QSTR(MP_QSTR_TX), MP_ROM_PTR(&pin_GPIO43) },
    { MP_ROM_QSTR(MP_QSTR_RX), MP_ROM_PTR(&pin_GPIO44) },

    { MP_ROM_QSTR(MP_QSTR_SDA), MP_ROM_PTR(DEFAULT_I2C_BUS_SDA) },
    { MP_ROM_QSTR(MP_QSTR_SCL), MP_ROM_PTR(DEFAULT_I2C_BUS_SCL) },

    { MP_ROM_QSTR(MP_QSTR_MISO), MP_ROM_PTR(DEFAULT_SPI_BUS_MISO) },
    { MP_ROM_QSTR(MP_QSTR_MOSI), MP_ROM_PTR(DEFAULT_SPI_BUS_MOSI) },
    { MP_ROM_QSTR(MP_QSTR_SCK), MP_ROM_PTR(DEFAULT_SPI_BUS_SCK) },
    { MP_ROM_QSTR(MP_QSTR_CS), MP_ROM_PTR(&pin_GPIO15) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_A0), MP_ROM_PTR(&pin_GPIO17) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_A1), MP_ROM_PTR(&pin_GPIO16) },

    // I/O expander pin numbers
    { MP_ROM_QSTR(MP_QSTR_TFT_SCK), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_TFT_CS), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_TFT_RESET), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_TP_IRQ), MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_BACKLIGHT), MP_ROM_INT(4) },
    { MP_ROM_QSTR(MP_QSTR_BTN_UP), MP_ROM_INT(5) },
    { MP_ROM_QSTR(MP_QSTR_BTN_DN), MP_ROM_INT(6) },
    { MP_ROM_QSTR(MP_QSTR_TFT_MOSI), MP_ROM_INT(7) },

    { MP_ROM_QSTR(MP_QSTR_I2C), MP_ROM_PTR(&board_i2c_obj) },
    { MP_ROM_QSTR(MP_QSTR_STEMMA_I2C), MP_ROM_PTR(&board_i2c_obj) },
    { MP_ROM_QSTR(MP_QSTR_SPI), MP_ROM_PTR(&board_spi_obj) },
};
MP_DEFINE_CONST_DICT(board_module_globals, board_module_globals_table);
