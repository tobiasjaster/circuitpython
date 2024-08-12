// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "py/mphal.h"
#include "py/obj.h"
#include "py/runtime.h"

#include "common-hal/microcontroller/__init__.h"
#if CIRCUITPY_NVM
#include "shared-bindings/nvm/ByteArray.h"
#endif
#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/microcontroller/Processor.h"
#include "supervisor/filesystem.h"
#include "supervisor/port.h"
#include "supervisor/shared/safe_mode.h"

#include "src/rp2040/hardware_structs/include/hardware/structs/sio.h"
#include "src/rp2_common/hardware_sync/include/hardware/sync.h"

#include "hardware/watchdog.h"

void common_hal_mcu_delay_us(uint32_t delay) {
    mp_hal_delay_us(delay);
}

volatile uint32_t nesting_count = 0;
void common_hal_mcu_disable_interrupts(void) {
    // We don't use save_and_disable_interrupts() from the sdk because we don't want to worry about PRIMASK.
    // This is what we do on the SAMD21 via CMSIS.
    asm volatile ("cpsid i" : : : "memory");
    __dmb();
    nesting_count++;
}

void common_hal_mcu_enable_interrupts(void) {
    if (nesting_count == 0) {
        // reset_into_safe_mode(SAFE_MODE_INTERRUPT_ERROR);
    }
    nesting_count--;
    if (nesting_count > 0) {
        return;
    }
    __dmb();
    asm volatile ("cpsie i" : : : "memory");
}

static bool next_reset_to_bootloader = false;

void common_hal_mcu_on_next_reset(mcu_runmode_t runmode) {
    switch (runmode) {
        case RUNMODE_UF2:
        case RUNMODE_BOOTLOADER:
            next_reset_to_bootloader = true;
            break;
        case RUNMODE_SAFE_MODE:
            safe_mode_on_next_reset(SAFE_MODE_PROGRAMMATIC);
            break;
        default:
            break;
    }
}

void common_hal_mcu_reset(void) {
    filesystem_flush();
    if (next_reset_to_bootloader) {
        reset_to_bootloader();
    } else {
        reset_cpu();
    }
}

// The singleton microcontroller.Processor object, bound to microcontroller.cpu
// It currently only has properties, and no state.
#if CIRCUITPY_PROCESSOR_COUNT > 1
static const mcu_processor_obj_t processor0 = {
    .base = {
        .type = &mcu_processor_type,
    },
};

static const mcu_processor_obj_t processor1 = {
    .base = {
        .type = &mcu_processor_type,
    },
};

const mp_rom_obj_tuple_t common_hal_multi_processor_obj = {
    {&mp_type_tuple},
    CIRCUITPY_PROCESSOR_COUNT,
    {
        MP_ROM_PTR(&processor0),
        MP_ROM_PTR(&processor1)
    }
};
#endif

const mcu_processor_obj_t common_hal_mcu_processor_obj = {
    .base = {
        .type = &mcu_processor_type,
    },
};

#if CIRCUITPY_NVM && CIRCUITPY_INTERNAL_NVM_SIZE > 0
// The singleton nvm.ByteArray object.
const nvm_bytearray_obj_t common_hal_mcu_nvm_obj = {
    .base = {
        .type = &nvm_bytearray_type,
    },
    .len = CIRCUITPY_INTERNAL_NVM_SIZE,
    .start_address = (uint8_t *)(CIRCUITPY_INTERNAL_NVM_START_ADDR)
};
#endif

#if CIRCUITPY_WATCHDOG
// The singleton watchdog.WatchDogTimer object.
watchdog_watchdogtimer_obj_t common_hal_mcu_watchdogtimer_obj = {
    .base = {
        .type = &watchdog_watchdogtimer_type,
    },
    .timeout = 0.0f,
    .mode = WATCHDOGMODE_NONE,
};
#endif

// This maps MCU pin names to pin objects.
const mp_rom_map_elem_t mcu_pin_global_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_GPIO0), MP_ROM_PTR(&pin_GPIO0) },
    { MP_ROM_QSTR(MP_QSTR_GPIO1), MP_ROM_PTR(&pin_GPIO1) },
    { MP_ROM_QSTR(MP_QSTR_GPIO2), MP_ROM_PTR(&pin_GPIO2) },
    { MP_ROM_QSTR(MP_QSTR_GPIO3), MP_ROM_PTR(&pin_GPIO3) },
    { MP_ROM_QSTR(MP_QSTR_GPIO4), MP_ROM_PTR(&pin_GPIO4) },
    { MP_ROM_QSTR(MP_QSTR_GPIO5), MP_ROM_PTR(&pin_GPIO5) },
    { MP_ROM_QSTR(MP_QSTR_GPIO6), MP_ROM_PTR(&pin_GPIO6) },
    { MP_ROM_QSTR(MP_QSTR_GPIO7), MP_ROM_PTR(&pin_GPIO7) },
    { MP_ROM_QSTR(MP_QSTR_GPIO8), MP_ROM_PTR(&pin_GPIO8) },
    { MP_ROM_QSTR(MP_QSTR_GPIO9), MP_ROM_PTR(&pin_GPIO9) },
    { MP_ROM_QSTR(MP_QSTR_GPIO10), MP_ROM_PTR(&pin_GPIO10) },
    { MP_ROM_QSTR(MP_QSTR_GPIO11), MP_ROM_PTR(&pin_GPIO11) },
    { MP_ROM_QSTR(MP_QSTR_GPIO12), MP_ROM_PTR(&pin_GPIO12) },
    { MP_ROM_QSTR(MP_QSTR_GPIO13), MP_ROM_PTR(&pin_GPIO13) },
    { MP_ROM_QSTR(MP_QSTR_GPIO14), MP_ROM_PTR(&pin_GPIO14) },
    { MP_ROM_QSTR(MP_QSTR_GPIO15), MP_ROM_PTR(&pin_GPIO15) },
    { MP_ROM_QSTR(MP_QSTR_GPIO16), MP_ROM_PTR(&pin_GPIO16) },
    { MP_ROM_QSTR(MP_QSTR_GPIO17), MP_ROM_PTR(&pin_GPIO17) },
    { MP_ROM_QSTR(MP_QSTR_GPIO18), MP_ROM_PTR(&pin_GPIO18) },
    { MP_ROM_QSTR(MP_QSTR_GPIO19), MP_ROM_PTR(&pin_GPIO19) },
    { MP_ROM_QSTR(MP_QSTR_GPIO20), MP_ROM_PTR(&pin_GPIO20) },
    { MP_ROM_QSTR(MP_QSTR_GPIO21), MP_ROM_PTR(&pin_GPIO21) },
    { MP_ROM_QSTR(MP_QSTR_GPIO22), MP_ROM_PTR(&pin_GPIO22) },
    #if !defined(IGNORE_GPIO23)
    { MP_ROM_QSTR(MP_QSTR_GPIO23), MP_ROM_PTR(&pin_GPIO23) },
    #endif
    #if !defined(IGNORE_GPIO24)
    { MP_ROM_QSTR(MP_QSTR_GPIO24), MP_ROM_PTR(&pin_GPIO24) },
    #endif
    #if !defined(IGNORE_GPIO25)
    { MP_ROM_QSTR(MP_QSTR_GPIO25), MP_ROM_PTR(&pin_GPIO25) },
    #endif
    { MP_ROM_QSTR(MP_QSTR_GPIO26), MP_ROM_PTR(&pin_GPIO26) },
    { MP_ROM_QSTR(MP_QSTR_GPIO27), MP_ROM_PTR(&pin_GPIO27) },
    { MP_ROM_QSTR(MP_QSTR_GPIO28), MP_ROM_PTR(&pin_GPIO28) },
    { MP_ROM_QSTR(MP_QSTR_GPIO29), MP_ROM_PTR(&pin_GPIO29) },
    #if NUM_BANK0_GPIOS == 48
    { MP_ROM_QSTR(MP_QSTR_GPIO30), MP_ROM_PTR(&pin_GPIO30) },
    { MP_ROM_QSTR(MP_QSTR_GPIO31), MP_ROM_PTR(&pin_GPIO31) },
    { MP_ROM_QSTR(MP_QSTR_GPIO32), MP_ROM_PTR(&pin_GPIO32) },
    { MP_ROM_QSTR(MP_QSTR_GPIO33), MP_ROM_PTR(&pin_GPIO33) },
    { MP_ROM_QSTR(MP_QSTR_GPIO34), MP_ROM_PTR(&pin_GPIO34) },
    { MP_ROM_QSTR(MP_QSTR_GPIO35), MP_ROM_PTR(&pin_GPIO35) },
    { MP_ROM_QSTR(MP_QSTR_GPIO36), MP_ROM_PTR(&pin_GPIO36) },
    { MP_ROM_QSTR(MP_QSTR_GPIO37), MP_ROM_PTR(&pin_GPIO37) },
    { MP_ROM_QSTR(MP_QSTR_GPIO38), MP_ROM_PTR(&pin_GPIO38) },
    { MP_ROM_QSTR(MP_QSTR_GPIO39), MP_ROM_PTR(&pin_GPIO39) },
    { MP_ROM_QSTR(MP_QSTR_GPIO40), MP_ROM_PTR(&pin_GPIO40) },
    { MP_ROM_QSTR(MP_QSTR_GPIO41), MP_ROM_PTR(&pin_GPIO41) },
    { MP_ROM_QSTR(MP_QSTR_GPIO42), MP_ROM_PTR(&pin_GPIO42) },
    { MP_ROM_QSTR(MP_QSTR_GPIO43), MP_ROM_PTR(&pin_GPIO43) },
    { MP_ROM_QSTR(MP_QSTR_GPIO44), MP_ROM_PTR(&pin_GPIO44) },
    { MP_ROM_QSTR(MP_QSTR_GPIO45), MP_ROM_PTR(&pin_GPIO45) },
    { MP_ROM_QSTR(MP_QSTR_GPIO46), MP_ROM_PTR(&pin_GPIO46) },
    { MP_ROM_QSTR(MP_QSTR_GPIO47), MP_ROM_PTR(&pin_GPIO47) },
    #endif
    #if CIRCUITPY_CYW43
    { MP_ROM_QSTR(MP_QSTR_CYW0), MP_ROM_PTR(&pin_CYW0) },
    { MP_ROM_QSTR(MP_QSTR_CYW1), MP_ROM_PTR(&pin_CYW1) },
    { MP_ROM_QSTR(MP_QSTR_CYW2), MP_ROM_PTR(&pin_CYW2) },
    #endif
};
MP_DEFINE_CONST_DICT(mcu_pin_globals, mcu_pin_global_dict_table);

const mcu_pin_obj_t *mcu_get_pin_by_number(int number) {
    for (size_t i = 0; i < MP_ARRAY_SIZE(mcu_pin_global_dict_table); i++) {
        mcu_pin_obj_t *obj = MP_OBJ_TO_PTR(mcu_pin_global_dict_table[i].value);
        if (obj->base.type == &mcu_pin_type && obj->number == number) {
            return obj;
        }
    }
    return NULL;
}
