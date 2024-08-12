// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 microDev
// SPDX-FileCopyrightText: Copyright (c) 2023 Bob Abeles
//
// SPDX-License-Identifier: MIT

#include <string.h>

#include "shared-bindings/memorymap/AddressRange.h"

#include "py/runtime.h"

#include "hardware/regs/addressmap.h"

// RP2 address map ranges, must be arranged in order by ascending start address
#ifdef PICO_RP2040
addressmap_rp2_range_t rp2_ranges[] = {
    {(uint8_t *)ROM_BASE,           0x00004000, ROM},        // boot ROM
    {(uint8_t *)XIP_BASE,           0x00100000, XIP},        // XIP normal cache operation
    {(uint8_t *)XIP_NOALLOC_BASE,   0x00100000, XIP},        // XIP check for hit, no update on miss
    {(uint8_t *)XIP_NOCACHE_BASE,   0x00100000, XIP},        // XIP don't check for hit, no update on miss
    {(uint8_t *)XIP_NOCACHE_NOALLOC_BASE, 0x00100000, XIP},  // XIP bypass cache completely
    {(uint8_t *)XIP_CTRL_BASE,      0x00004000, IO},         // XIP control registers
    {(uint8_t *)XIP_SRAM_BASE,      0x00004000, SRAM},       // XIP SRAM 16KB XIP cache
    {(uint8_t *)XIP_SSI_BASE,       0x00004000, IO},         // XIP SSI registers
    {(uint8_t *)SRAM_BASE,          0x00042000, SRAM},       // SRAM 256KB striped plus 16KB contiguous
    {(uint8_t *)SRAM0_BASE,         0x00040000, SRAM},       // SRAM0 to SRAM3 256KB non-striped
    {(uint8_t *)SYSINFO_BASE,       0x00070000, IO},         // APB peripherals
    {(uint8_t *)DMA_BASE,           0x00004000, IO},         // DMA registers
    {(uint8_t *)USBCTRL_DPRAM_BASE, 0x00001000, SRAM},       // USB DPSRAM 4KB
    {(uint8_t *)USBCTRL_REGS_BASE,  0x00004000, IO},         // USB registers
    {(uint8_t *)PIO0_BASE,          0x00004000, IO},         // PIO0 registers
    {(uint8_t *)PIO1_BASE,          0x00004000, IO},         // PIO1 registers
    {(uint8_t *)SIO_BASE,           0x00001000, IO},         // SIO registers, no aliases
    {(uint8_t *)PPB_BASE,           0x00004000, IO}          // PPB registers
};
#endif
#ifdef PICO_RP2350
addressmap_rp2_range_t rp2_ranges[] = {
    {(uint8_t *)ROM_BASE,           0x00004000, ROM},        // boot ROM
    {(uint8_t *)XIP_BASE,           0x00100000, XIP},        // XIP normal cache operation
    {(uint8_t *)XIP_NOCACHE_NOALLOC_BASE, 0x00100000, XIP},  // XIP bypass cache completely
    {(uint8_t *)XIP_MAINTENANCE_BASE,     0x00100000, XIP},  // XIP cache maintenance based on lower 3 address bits. Data is ignored
    {(uint8_t *)XIP_NOCACHE_NOALLOC_NOTRANSLATE_BASE, 0x00100000, XIP},  // XIP skip cache and address translation
    {(uint8_t *)SRAM_BASE,          SRAM_END - SRAM_BASE, SRAM},       // SRAM 256KB striped plus 16KB contiguous
    {(uint8_t *)SYSINFO_BASE,       0x00070000, IO},         // APB peripherals
    {(uint8_t *)XIP_CTRL_BASE,      0x00004000, IO},         // XIP control registers
    {(uint8_t *)XIP_QMI_BASE,       0x00004000, IO},         // XIP QMI registers
    {(uint8_t *)DMA_BASE,           0x00004000, IO},         // DMA registers
    {(uint8_t *)USBCTRL_DPRAM_BASE, 0x00001000, SRAM},       // USB DPSRAM 4KB
    {(uint8_t *)USBCTRL_REGS_BASE,  0x00004000, IO},         // USB registers
    {(uint8_t *)PIO0_BASE,          0x00004000, IO},         // PIO0 registers
    {(uint8_t *)PIO1_BASE,          0x00004000, IO},         // PIO1 registers
    {(uint8_t *)PIO2_BASE,          0x00004000, IO},         // PIO2 registers
    {(uint8_t *)SIO_BASE,           0x00001000, IO},         // SIO registers, no aliases
    {(uint8_t *)PPB_BASE,           0x00004000, IO}          // PPB registers
};
#endif

void common_hal_memorymap_addressrange_construct(memorymap_addressrange_obj_t *self,
    uint8_t *start_address, size_t length) {
    for (size_t i = 0; i < MP_ARRAY_SIZE(rp2_ranges); i++) {
        if (start_address <= rp2_ranges[i].start_address) {
            uint8_t *range_end_address = rp2_ranges[i].start_address + rp2_ranges[i].len - 1;
            uint8_t *end_address = start_address + length - 1;
            if (start_address > range_end_address || end_address > range_end_address) {
                break;
            }
            self->start_address = start_address;
            self->len = length;
            self->type = rp2_ranges[i].type;
            return;
        }
    }

    mp_raise_ValueError(MP_ERROR_TEXT("Address range not allowed"));
}

size_t common_hal_memorymap_addressrange_get_length(const memorymap_addressrange_obj_t *self) {
    return self->len;
}

void common_hal_memorymap_addressrange_set_bytes(const memorymap_addressrange_obj_t *self,
    size_t start_index, uint8_t *values, size_t len) {
    uint8_t *dest_addr = self->start_address + start_index;
    switch (self->type) {
        case SRAM:
            // Writes to SRAM may be arbitrary length and alignment. We use memcpy() which
            // may optimize aligned writes depending on CIRCUITPY_FULL_BUILD of the CP build.
            memcpy(dest_addr, values, len);
            break;
        case IO:
            if ((size_t)dest_addr & 0x03 || len & 0x03) {
                // Unaligned access or unaligned length not supported by RP2 for IO registers
                mp_raise_RuntimeError(MP_ERROR_TEXT("Unable to access unaligned IO register"));
            } else {
                // Aligned access and length, use 32-bit writes
                uint32_t *dest_addr32 = (uint32_t *)dest_addr;
                size_t access_count = len >> 2;
                for (size_t i = 0; i < access_count; i++) {
                    *dest_addr32++ = ((uint32_t *)values)[i];
                }
            }
            break;
        case XIP:
        case ROM:
            // XIP and ROM are read-only
            mp_raise_RuntimeError(MP_ERROR_TEXT("Unable to write to read-only memory"));
            break;
    }
}

void common_hal_memorymap_addressrange_get_bytes(const memorymap_addressrange_obj_t *self,
    size_t start_index, size_t len, uint8_t *values) {
    uint8_t *src_addr = self->start_address + start_index;
    switch (self->type) {
        case SRAM:
        case XIP:
        case ROM:
            // Reads from these sources may be arbitrary length and alignment. We use memcpy()
            // which may optimize aligned writes depending on CIRCUITPY_FULL_BUILD of the CP build.
            memcpy(values, src_addr, len);
            break;
        case IO:
            if ((size_t)src_addr & 0x03 || len & 0x03) {
                // Unaligned access or unaligned length not supported by RP2 for IO registers
                mp_raise_RuntimeError(MP_ERROR_TEXT("Unable to access unaligned IO register"));
            } else {
                // Aligned access and length, use 32-bit reads
                uint32_t *src_addr32 = (uint32_t *)src_addr;
                size_t access_count = len >> 2;
                for (size_t i = 0; i < access_count; i++) {
                    ((uint32_t *)values)[i] = *src_addr32++;
                }
            }
            break;
    }
}
