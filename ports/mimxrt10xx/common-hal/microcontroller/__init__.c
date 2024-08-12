// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2016 Scott Shawcroft for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2019 Artur Pacholec
//
// SPDX-License-Identifier: MIT
// TODO
#include "py/mphal.h"
#include "py/obj.h"
#include "py/runtime.h"

#include "fsl_device_registers.h"

#include "reset.h"

#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/microcontroller/Processor.h"
#include "supervisor/shared/safe_mode.h"

#include "pins.h"

void common_hal_mcu_delay_us(uint32_t delay) {
    mp_hal_delay_us(delay);
}

volatile uint32_t PLACE_IN_DTCM_BSS(nesting_count) = 0;
void PLACE_IN_ITCM(common_hal_mcu_disable_interrupts)(void) {
    __disable_irq();
    __DMB();
    nesting_count++;
}

void PLACE_IN_ITCM(common_hal_mcu_enable_interrupts)(void) {
    if (nesting_count == 0) {
        // This is very very bad because it means there was mismatched disable/enables
        reset_into_safe_mode(SAFE_MODE_INTERRUPT_ERROR);
    }
    nesting_count--;
    if (nesting_count > 0) {
        return;
    }
    __DMB();
    __enable_irq();
}

void common_hal_mcu_on_next_reset(mcu_runmode_t runmode) {
    if (runmode == RUNMODE_BOOTLOADER) {
        if (!bootloader_available()) {
            mp_raise_ValueError(MP_ERROR_TEXT("No bootloader present"));
        }
        // Pretend to be the first of the two reset presses needed to enter the
        // bootloader. That way one reset will end in the bootloader.
        DBL_TAP_REG = DBL_TAP_MAGIC;
    } else {
        // Set up the default.
        DBL_TAP_REG = DBL_TAP_MAGIC_QUICK_BOOT;
    }
    if (runmode == RUNMODE_SAFE_MODE) {
        safe_mode_on_next_reset(SAFE_MODE_PROGRAMMATIC);
    }
}

void common_hal_mcu_reset(void) {
    NVIC_SystemReset();
}

// The singleton microcontroller.Processor object, bound to microcontroller.cpu
// It currently only has properties, and no state.
const mcu_processor_obj_t common_hal_mcu_processor_obj = {
    .base = {
        .type = &mcu_processor_type,
    },
};

// NVM is only available on Express boards for now.
#if CIRCUITPY_INTERNAL_NVM_SIZE > 0
// The singleton nvm.ByteArray object.
const nvm_bytearray_obj_t common_hal_mcu_nvm_obj = {
    .base = {
        .type = &nvm_bytearray_type,
    },
    .len = CIRCUITPY_INTERNAL_NVM_SIZE,
    .start_address = (uint8_t *)(FLASH_SIZE - CIRCUITPY_INTERNAL_NVM_SIZE)
};
#endif

// This maps MCU pin names to pin objects.
// NOTE: for all i.MX chips, order MUST match _iomuxc_sw_mux_ctl_pad enum
static const mp_rom_map_elem_t mcu_pin_global_dict_table[PIN_COUNT] = {

#define FORMAT_PIN(pin_name) { MP_ROM_QSTR(MP_QSTR_##pin_name), MP_ROM_PTR(&pin_##pin_name) },
    #include "pin_names.h"
#undef FORMAT_PIN
};
MP_DEFINE_CONST_DICT(mcu_pin_globals, mcu_pin_global_dict_table);
