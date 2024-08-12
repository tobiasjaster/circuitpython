// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2013, 2014 Damien P. George
// SPDX-FileCopyrightText: Copyright (c) 2019, Lucian Copeland for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <stdint.h>
#include <string.h>


#include "py/mperrno.h"
#include "py/runtime.h"



#include "shared-bindings/analogio/AnalogOut.h"
#include "shared-bindings/microcontroller/Pin.h"

#include "common-hal/microcontroller/Pin.h"

#include STM32_HAL_H

#ifndef __HAL_RCC_DAC_CLK_ENABLE
#define __HAL_RCC_DAC_CLK_ENABLE __HAL_RCC_DAC1_CLK_ENABLE
#endif
#ifndef __HAL_RCC_DAC_CLK_DISABLE
#define __HAL_RCC_DAC_CLK_DISABLE __HAL_RCC_DAC1_CLK_DISABLE
#endif

// DAC is shared between both channels.
#if HAS_DAC
DAC_HandleTypeDef handle;
#endif

static bool dac_on[2];

void common_hal_analogio_analogout_construct(analogio_analogout_obj_t *self,
    const mcu_pin_obj_t *pin) {
    #if !(HAS_DAC)
    mp_raise_ValueError(MP_ERROR_TEXT("No DAC on chip"));
    #else
    if (pin == &pin_PA04) {
        self->channel = DAC_CHANNEL_1;
        self->dac_index = 0;
    } else if (pin == &pin_PA05) {
        self->channel = DAC_CHANNEL_2;
        self->dac_index = 1;
    } else {
        raise_ValueError_invalid_pin();
    }

    // Only init if the shared DAC is empty or reset
    if (handle.Instance == NULL || handle.State == HAL_DAC_STATE_RESET) {
        __HAL_RCC_DAC_CLK_ENABLE();
        handle.Instance = DAC;
        if (HAL_DAC_Init(&handle) != HAL_OK) {
            mp_raise_ValueError(MP_ERROR_TEXT("DAC Device Init Error"));
        }
    }

    // init channel specific pin
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pin_mask(pin->number);
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(pin_port(pin->port), &GPIO_InitStruct);

    self->ch_handle.DAC_Trigger = DAC_TRIGGER_NONE;
    self->ch_handle.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    if (HAL_DAC_ConfigChannel(&handle, &self->ch_handle, self->channel) != HAL_OK) {
        mp_raise_ValueError(MP_ERROR_TEXT("DAC Channel Init Error"));
    }

    dac_on[self->dac_index] = true;
    self->pin = pin;
    common_hal_mcu_pin_claim(pin);
    #endif
}

bool common_hal_analogio_analogout_deinited(analogio_analogout_obj_t *self) {
    return !dac_on[self->dac_index];
}

void common_hal_analogio_analogout_deinit(analogio_analogout_obj_t *self) {
    #if HAS_DAC
    reset_pin_number(self->pin->port, self->pin->number);
    self->pin = NULL;
    dac_on[self->dac_index] = false;

    // turn off the DAC if both channels are off
    if (dac_on[0] == false && dac_on[1] == false) {
        __HAL_RCC_DAC_CLK_DISABLE();
        HAL_DAC_DeInit(&handle);
    }
    #endif
}

void common_hal_analogio_analogout_set_value(analogio_analogout_obj_t *self,
    uint16_t value) {
    #if HAS_DAC
    HAL_DAC_SetValue(&handle, self->channel, DAC_ALIGN_12B_R, value >> 4);
    HAL_DAC_Start(&handle, self->channel);
    #endif
}

void analogout_reset(void) {
    #if HAS_DAC
    __HAL_RCC_DAC_CLK_DISABLE();
    HAL_DAC_DeInit(&handle);
    #endif
}
