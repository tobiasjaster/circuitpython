// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Lucian Copeland for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2019 Artur Pacholec
// SPDX-FileCopyrightText: Copyright (c) 2023 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

/*
 * This file is autogenerated! Do NOT hand edit it. Instead, edit tools/gen_peripherals_data.py and
 * then rerun the script. You'll need to 1) clone https://github.com/nxp-mcuxpresso/mcux-soc-svd/
 * and 2) download and extract config tools data from https://mcuxpresso.nxp.com/en/select_config_tools_data.
 *
 * Run `python tools/gen_peripherals_data.py <svd dir> <config dir> MIMXRT1011` to update this file.
 */

#pragma once
extern LPI2C_Type *const mcu_i2c_banks[2];
extern const mcu_periph_obj_t mcu_i2c_sda_list[8];
extern const mcu_periph_obj_t mcu_i2c_scl_list[8];

extern LPSPI_Type *const mcu_spi_banks[2];
extern const mcu_periph_obj_t mcu_spi_sck_list[4];
extern const mcu_periph_obj_t mcu_spi_sdo_list[4];
extern const mcu_periph_obj_t mcu_spi_sdi_list[4];

extern LPUART_Type *const mcu_uart_banks[4];
extern const mcu_periph_obj_t mcu_uart_rx_list[9];
extern const mcu_periph_obj_t mcu_uart_tx_list[9];
extern const mcu_periph_obj_t mcu_uart_rts_list[4];
extern const mcu_periph_obj_t mcu_uart_cts_list[4];

extern I2S_Type *const mcu_i2s_banks[2];
extern const mcu_periph_obj_t mcu_i2s_rx_data0_list[2];
extern const mcu_periph_obj_t mcu_i2s_rx_sync_list[2];
extern const mcu_periph_obj_t mcu_i2s_tx_bclk_list[2];
extern const mcu_periph_obj_t mcu_i2s_tx_data0_list[2];
extern const mcu_periph_obj_t mcu_i2s_tx_sync_list[2];
extern const mcu_periph_obj_t mcu_i2s_mclk_list[2];

extern const mcu_periph_obj_t mcu_mqs_left_list[1];
extern const mcu_periph_obj_t mcu_mqs_right_list[1];

extern const mcu_pwm_obj_t mcu_pwm_list[20];
