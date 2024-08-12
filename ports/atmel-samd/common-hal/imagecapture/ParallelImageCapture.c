// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 Jeff Epler for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "py/obj.h"
#include "py/runtime.h"

#include "shared/runtime/context_manager_helpers.h"
#include "shared/runtime/interrupt_char.h"

#include "shared-bindings/imagecapture/ParallelImageCapture.h"
#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "common-hal/imagecapture/ParallelImageCapture.h"

#include "hal/include/hal_gpio.h"
#include "atmel_start_pins.h"

#include "audio_dma.h"
#include "samd/clocks.h"
#include "samd/events.h"

#define GPIO_PIN_FUNCTION_PCC (GPIO_PIN_FUNCTION_K)

#define PIN_PCC_D0 (PIN_PA16)
#define PIN_PCC_DEN1 (PIN_PA12)
#define PIN_PCC_DEN2 (PIN_PA13)
#define PIN_PCC_CLK (PIN_PA14)

void common_hal_imagecapture_parallelimagecapture_construct(imagecapture_parallelimagecapture_obj_t *self,
    const uint8_t data_pins[],
    uint8_t data_count,
    const mcu_pin_obj_t *data_clock,
    const mcu_pin_obj_t *vertical_sync,
    const mcu_pin_obj_t *horizontal_reference) {

    for (int i = 0; i < data_count; i++) {
        if (data_pins[i] != PIN_PCC_D0 + i) {
            mp_raise_ValueError_varg(MP_ERROR_TEXT("Invalid data_pins[%d]"), i);
        }
    }
    // The peripheral supports 8, 10, 12, or 14 data bits, but the code only supports 8 at present
    if (data_count != 8) {
        mp_arg_error_invalid(MP_QSTR_datacount);
    }
    if (vertical_sync && vertical_sync->number != PIN_PCC_DEN1) {
        raise_ValueError_invalid_pin_name(MP_QSTR_vsync);
    }
    if (horizontal_reference && horizontal_reference->number != PIN_PCC_DEN2) {
        raise_ValueError_invalid_pin_name(MP_QSTR_href);
    }
    if (data_clock->number != PIN_PCC_CLK) {
        raise_ValueError_invalid_pin_name(MP_QSTR_data_clock);
    }
    // technically, 0 was validated as free already but check again
    for (int i = 0; i < data_count; i++) {
        if (!pin_number_is_free(data_pins[i])) {
            mp_raise_ValueError_varg(MP_ERROR_TEXT("data pin #%d in use"), i);
        }
    }

    PCC->MR.bit.PCEN = 0; // Make sure PCC is disabled before setting MR reg

    PCC->IDR.reg = 0b1111;       // Disable all PCC interrupts
    MCLK->APBDMASK.bit.PCC_ = 1; // Enable PCC clock

    // Accumulate 4 bytes into RHR register (two 16-bit pixels)
    PCC->MR.reg = PCC_MR_CID(0x1) | // Clear on falling DEN1 (VSYNC)
        PCC_MR_ISIZE(0x0) |         // Input data bus is 8 bits
        PCC_MR_DSIZE(0x2);          // "4 data" at a time (accumulate in RHR)

    PCC->MR.bit.PCEN = 1; // Enable PCC


    // Now we know we can allocate all pins
    self->data_count = data_count;
    self->vertical_sync = vertical_sync ? vertical_sync->number : NO_PIN;
    self->horizontal_reference = horizontal_reference ? horizontal_reference->number : NO_PIN;
    gpio_set_pin_direction(PIN_PCC_CLK, GPIO_DIRECTION_IN);
    gpio_set_pin_pull_mode(PIN_PCC_CLK, GPIO_PULL_OFF);
    gpio_set_pin_function(PIN_PCC_CLK, GPIO_PIN_FUNCTION_PCC);
    // claim_pin_number(PIN_PCC_CLK);
    if (vertical_sync) {
        gpio_set_pin_direction(PIN_PCC_DEN1, GPIO_DIRECTION_IN);
        gpio_set_pin_pull_mode(PIN_PCC_DEN1, GPIO_PULL_OFF);
        gpio_set_pin_function(PIN_PCC_DEN1, GPIO_PIN_FUNCTION_PCC); // VSYNC
        // claim_pin_number(PIN_PCC_DEN1);
    }
    if (horizontal_reference) {
        gpio_set_pin_direction(PIN_PCC_DEN2, GPIO_DIRECTION_IN);
        gpio_set_pin_pull_mode(PIN_PCC_DEN2, GPIO_PULL_OFF);
        gpio_set_pin_function(PIN_PCC_DEN2, GPIO_PIN_FUNCTION_PCC); // HSYNC
        // claim_pin_number(PIN_PCC_DEN2);
    }
    for (int i = 0; i < data_count; i++) {
        gpio_set_pin_direction(PIN_PCC_D0 + i, GPIO_DIRECTION_IN);
        gpio_set_pin_pull_mode(PIN_PCC_D0 + i, GPIO_PULL_OFF);
        gpio_set_pin_function(PIN_PCC_D0 + i, GPIO_PIN_FUNCTION_PCC);
        // claim_pin_number(PIN_PCC_D0+i);
    }
}

void common_hal_imagecapture_parallelimagecapture_deinit(imagecapture_parallelimagecapture_obj_t *self) {
    if (common_hal_imagecapture_parallelimagecapture_deinited(self)) {
        return;
    }
    reset_pin_number(self->vertical_sync);
    reset_pin_number(self->horizontal_reference);
    reset_pin_number(PIN_PCC_CLK);
    for (int i = 0; i < self->data_count; i++) {
        reset_pin_number(PIN_PCC_D0 + i);
    }
    self->data_count = 0;
}

bool common_hal_imagecapture_parallelimagecapture_deinited(imagecapture_parallelimagecapture_obj_t *self) {
    return self->data_count == 0;
}

static void setup_dma(DmacDescriptor *descriptor, size_t count, uint32_t *buffer) {
    descriptor->BTCTRL.reg = DMAC_BTCTRL_VALID |
        DMAC_BTCTRL_BLOCKACT_NOACT |
        DMAC_BTCTRL_EVOSEL_BLOCK |
        DMAC_BTCTRL_DSTINC |
        DMAC_BTCTRL_BEATSIZE_WORD;
    descriptor->BTCNT.reg = count;
    descriptor->DSTADDR.reg = (uint32_t)buffer + 4 * count;
    descriptor->SRCADDR.reg = (uint32_t)&PCC->RHR.reg;
    descriptor->DESCADDR.reg = 0;
}

void common_hal_imagecapture_parallelimagecapture_singleshot_capture(imagecapture_parallelimagecapture_obj_t *self, mp_obj_t buffer) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buffer, &bufinfo, MP_BUFFER_RW);

    uint8_t dma_channel = dma_allocate_channel();

    uint32_t *dest = bufinfo.buf;
    size_t count = bufinfo.len / 4; // PCC receives 4 bytes (2 pixels) at a time

    turn_on_event_system();

    setup_dma(dma_descriptor(dma_channel), count, dest);
    dma_configure(dma_channel, PCC_DMAC_ID_RX, true);

    if (self->vertical_sync) {
        const volatile uint32_t *vsync_reg = &PORT->Group[(self->vertical_sync / 32)].IN.reg;
        uint32_t vsync_bit = 1 << (self->vertical_sync % 32);

        while (*vsync_reg & vsync_bit) {
            // Wait for VSYNC low (frame end)

            RUN_BACKGROUND_TASKS;
            // Allow user to break out of a timeout with a KeyboardInterrupt.
            if (mp_hal_is_interrupted()) {
                dma_free_channel(dma_channel);
                return;
            }
        }
    }

    dma_enable_channel(dma_channel);

    while (DMAC->Channel[dma_channel].CHCTRLA.bit.ENABLE) {
        RUN_BACKGROUND_TASKS;
        if (mp_hal_is_interrupted()) {
            break;
        }
    }

    dma_disable_channel(dma_channel);
    dma_free_channel(dma_channel);
}
