// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Lucian Copeland for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2019 Artur Pacholec
//
// SPDX-License-Identifier: MIT

#pragma once

#include "pins.h"

typedef struct {
    uint8_t bank_idx : 4; // e.g. the peripheral number
    uint8_t mux_mode : 4;
    uint32_t input_reg;
    uint8_t input_idx;
    const mcu_pin_obj_t *pin;
} mcu_periph_obj_t;

#define PERIPH_PIN(p_bank_idx, p_mux_mode, p_input_reg, p_input_idx, p_pin) \
    { \
        .bank_idx = p_bank_idx, \
        .mux_mode = p_mux_mode, \
        .input_reg = p_input_reg == 0 ? 0 : (uint32_t)&(IOMUXC->SELECT_INPUT[p_input_reg]), \
        .input_idx = p_input_idx, \
        .pin = p_pin, \
    }

typedef struct {
    PWM_Type *pwm;
    pwm_submodule_t submodule : 4;
    pwm_channels_t channel : 4;
    uint8_t mux_mode;
    uint8_t input_idx;
    uint32_t input_reg;
    const mcu_pin_obj_t *pin;
} mcu_pwm_obj_t;

#define PWM_PIN(p_pwm, p_submodule, p_channel, p_iomuxc, p_pin) \
    PWM_PIN_(p_pwm, p_submodule, p_channel, p_iomuxc, p_pin)
// ----------------------------------------------------------//
// supplied by the expansion of p_iomuxc into multiple args //
#define PWM_PIN_(p_pwm, p_submodule, p_channel, p_mux_reg, p_mux_mode, p_input_reg, p_input_idx, p_config_reg, p_pin) \
    { \
        .pwm = p_pwm, \
        .submodule = p_submodule, \
        .channel = p_channel, \
        .mux_mode = p_mux_mode, \
        .input_reg = p_input_reg, \
        .input_idx = p_input_idx, \
        .pin = p_pin, \
    }

extern LPI2C_Type *const mcu_i2c_banks[];
extern LPSPI_Type *const mcu_spi_banks[];
extern LPUART_Type *const mcu_uart_banks[];

#ifdef MIMXRT1011_SERIES
#include "MIMXRT1011/periph.h"
#elif defined(MIMXRT1015_SERIES)
#include "MIMXRT1015/periph.h"
#elif defined(MIMXRT1021_SERIES)
#include "MIMXRT1021/periph.h"
#elif defined(MIMXRT1042_SERIES)
#include "MIMXRT1042/periph.h"
#elif defined(MIMXRT1052_SERIES)
#include "MIMXRT1052/periph.h"
#elif defined(MIMXRT1062_SERIES)
#include "MIMXRT1062/periph.h"
#elif defined(MIMXRT1176_cm7_SERIES)
#include "MIMXRT1176/periph.h"
#endif
