// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 microDev
//
// SPDX-License-Identifier: MIT

#include "common-hal/nvm/ByteArray.h"
#include "shared-bindings/nvm/ByteArray.h"

#include <string.h>

#include "py/runtime.h"
#include "src/rp2_common/hardware_flash/include/hardware/flash.h"
#include "shared-bindings/microcontroller/__init__.h"

extern uint32_t __flash_binary_start;
static const uint32_t flash_binary_start = (uint32_t)&__flash_binary_start;

#define RMV_OFFSET(addr) addr - flash_binary_start

uint32_t common_hal_nvm_bytearray_get_length(const nvm_bytearray_obj_t *self) {
    return self->len;
}

static void write_page(uint32_t page_addr, uint32_t offset, uint32_t len, uint8_t *bytes) {
    // Write a whole page to flash, buffering it first and then erasing and rewriting it
    // since we can only write a whole page at a time.
    if (offset == 0 && len == FLASH_PAGE_SIZE) {
        // disable interrupts to prevent core hang on rp2040
        common_hal_mcu_disable_interrupts();
        flash_range_program(RMV_OFFSET(page_addr), bytes, FLASH_PAGE_SIZE);
        common_hal_mcu_enable_interrupts();
    } else {
        uint8_t buffer[FLASH_PAGE_SIZE];
        memcpy(buffer, (uint8_t *)page_addr, FLASH_PAGE_SIZE);
        memcpy(buffer + offset, bytes, len);
        common_hal_mcu_disable_interrupts();
        flash_range_program(RMV_OFFSET(page_addr), buffer, FLASH_PAGE_SIZE);
        common_hal_mcu_enable_interrupts();
    }

}

static void erase_and_write_sector(uint32_t address, uint32_t len, uint8_t *bytes) {
    // Write a whole sector to flash, buffering it first and then erasing and rewriting it
    // since we can only erase a whole sector at a time.
    uint8_t buffer[FLASH_SECTOR_SIZE];
    #pragma GCC diagnostic push
    #if __GNUC__ >= 11
    // TODO: Update this to a better workaround for GCC 11 when one is provided.
    // See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99578#c20
    #pragma GCC diagnostic ignored "-Warray-bounds"
    #pragma GCC diagnostic ignored "-Wstringop-overread"
    #endif
    memcpy(buffer, (uint8_t *)CIRCUITPY_INTERNAL_NVM_START_ADDR, FLASH_SECTOR_SIZE);
    #pragma GCC diagnostic pop
    memcpy(buffer + address, bytes, len);
    // disable interrupts to prevent core hang on rp2040
    common_hal_mcu_disable_interrupts();
    flash_range_erase(RMV_OFFSET(CIRCUITPY_INTERNAL_NVM_START_ADDR), FLASH_SECTOR_SIZE);
    flash_range_program(RMV_OFFSET(CIRCUITPY_INTERNAL_NVM_START_ADDR), buffer, FLASH_SECTOR_SIZE);
    common_hal_mcu_enable_interrupts();
}

void common_hal_nvm_bytearray_get_bytes(const nvm_bytearray_obj_t *self,
    uint32_t start_index, uint32_t len, uint8_t *values) {
    memcpy(values, self->start_address + start_index, len);
}

bool common_hal_nvm_bytearray_set_bytes(const nvm_bytearray_obj_t *self,
    uint32_t start_index, uint8_t *values, uint32_t len) {
    uint8_t values_in[len];
    common_hal_nvm_bytearray_get_bytes(self, start_index, len, values_in);

    bool all_ones = true;
    for (uint32_t i = 0; i < len; i++) {
        if (values_in[i] != UINT8_MAX) {
            all_ones = false;
            break;
        }
    }

    if (all_ones) {
        uint32_t address = (uint32_t)self->start_address + start_index;
        uint32_t offset = address % FLASH_PAGE_SIZE;
        uint32_t page_addr = address - offset;

        while (len) {
            uint32_t write_len = MIN(len, FLASH_PAGE_SIZE - offset);
            write_page(page_addr, offset, write_len, values);
            len -= write_len;
            values += write_len;
            page_addr += FLASH_PAGE_SIZE;
            offset = 0;
        }
    } else {
        erase_and_write_sector(start_index, len, values);
    }

    return true;
}
