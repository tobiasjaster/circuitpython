// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Dan Halbert for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

// Definitions for which SAMD chip we're using.
#include "include/sam.h"

// Definitions that control circuitpy_mpconfig.h:

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef SAMD21

// HMCRAMC0_SIZE is defined in the ASF4 include files for each SAMD21 chip.
#define RAM_SIZE                                    HMCRAMC0_SIZE
#define BOOTLOADER_SIZE                             (8 * 1024)
#define CIRCUITPY_MCU_FAMILY                        samd21
#define MICROPY_PY_SYS_PLATFORM                     "Atmel SAMD21"
#define SPI_FLASH_MAX_BAUDRATE 8000000
#define MICROPY_PY_BUILTINS_COMPLEX                 (0)
#define MICROPY_PY_BUILTINS_NOTIMPLEMENTED          (0)
#define MICROPY_PY_FUNCTION_ATTRS                   (0)
#define MICROPY_PY_REVERSE_SPECIAL_METHODS          (0)
#define MICROPY_PY_COLLECTIONS_DEQUE                (0)
#define MICROPY_PY_COLLECTIONS_ORDEREDDICT          (0)
#define MICROPY_PY_ERRNO_LIST \
    X(EPERM) \
    X(ENOENT) \
    X(EIO) \
    X(EAGAIN) \
    X(ENOMEM) \
    X(EACCES) \
    X(EEXIST) \
    X(ENODEV) \
    X(EISDIR) \
    X(EINVAL) \

#define MICROPY_FATFS_EXFAT    (0)
// FAT32 mkfs takes about 500 bytes.
#define MICROPY_FATFS_MKFS_FAT32 (0)

// Only support simpler HID descriptors on SAMD21.
#define CIRCUITPY_USB_HID_MAX_REPORT_IDS_PER_DESCRIPTOR (1)

#endif // SAMD21

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef SAM_D5X_E5X

// HSRAM_SIZE is defined in the ASF4 include files for each SAM_D5X_E5X chip.
#define RAM_SIZE                                    HSRAM_SIZE
#define BOOTLOADER_SIZE                             (16 * 1024)
#define CIRCUITPY_MCU_FAMILY                        samd51
#ifdef SAMD51
#define MICROPY_PY_SYS_PLATFORM                     "MicroChip SAMD51"
#elif defined(SAME54)
#define MICROPY_PY_SYS_PLATFORM                     "MicroChip SAME54"
#endif
#define SPI_FLASH_MAX_BAUDRATE 24000000
#define MICROPY_PY_BUILTINS_NOTIMPLEMENTED          (1)
#define MICROPY_PY_FUNCTION_ATTRS                   (1)
//      MICROPY_PY_ERRNO_LIST - Use the default

#endif // SAM_D5X_E5X

////////////////////////////////////////////////////////////////////////////////////////////////////

// This also includes mpconfigboard.h.
#include "py/circuitpy_mpconfig.h"

// Definitions that can be overridden by mpconfigboard.h:

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef SAMD21

#ifndef CIRCUITPY_INTERNAL_FLASH_FILESYSTEM_SIZE
#if INTERNAL_FLASH_FILESYSTEM
#define CIRCUITPY_INTERNAL_FLASH_FILESYSTEM_SIZE (64 * 1024)
#else
#define CIRCUITPY_INTERNAL_FLASH_FILESYSTEM_SIZE (0)
#endif
#endif

#ifndef CIRCUITPY_INTERNAL_NVM_SIZE
#define CIRCUITPY_INTERNAL_NVM_SIZE (256)
#endif

#ifndef CIRCUITPY_DEFAULT_STACK_SIZE
#define CIRCUITPY_DEFAULT_STACK_SIZE                3584
#endif

#ifndef SAMD21_BOD33_LEVEL
// Set brownout detection to ~2.7V. Default from factory is 1.7V,
// which is too low for proper operation of external SPI flash chips
// (they are 2.7-3.6V).
#define SAMD21_BOD33_LEVEL (39)
// 2.77V with hysteresis off. Table 37.20 in datasheet.
#endif

// Smallest unit of flash that can be erased.
#define FLASH_ERASE_SIZE NVMCTRL_ROW_SIZE

#endif // SAMD21

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef SAM_D5X_E5X

#ifndef CIRCUITPY_INTERNAL_NVM_SIZE
#define CIRCUITPY_INTERNAL_NVM_SIZE (8192)
#endif

#ifndef CIRCUITPY_DEFAULT_STACK_SIZE
#define CIRCUITPY_DEFAULT_STACK_SIZE                (24 * 1024)
#endif

#ifndef SAMD5x_E5x_BOD33_LEVEL
// Set brownout detection to ~2.7V. Default from factory is 1.7V,
// which is too low for proper operation of external SPI flash chips
// (they are 2.7-3.6V).
#define SAMD5x_E5x_BOD33_LEVEL (200)
// 2.7V: 1.5V + LEVEL * 6mV.
#endif

// Smallest unit of flash that can be erased.
#define FLASH_ERASE_SIZE NVMCTRL_BLOCK_SIZE

// If CIRCUITPY is internal, use half of flash for it.
#if INTERNAL_FLASH_FILESYSTEM
  #ifndef CIRCUITPY_INTERNAL_FLASH_FILESYSTEM_SIZE
    #define CIRCUITPY_INTERNAL_FLASH_FILESYSTEM_SIZE (FLASH_SIZE / 2)
  #endif
#else
  #define CIRCUITPY_INTERNAL_FLASH_FILESYSTEM_SIZE (0)
#endif

#endif // SAM_D5X_E5X

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CALIBRATE_CRYSTALLESS
#define CALIBRATE_CRYSTALLESS (0)
#endif

#ifndef BOARD_HAS_CRYSTAL
#define BOARD_HAS_CRYSTAL (0)
#endif

#ifndef BOARD_XOSC_FREQ_HZ
// 0 Indicates XOSC is not used.
  #define BOARD_XOSC_FREQ_HZ (0)
#else
// For now, only allow external clock sources that divide cleanly into
// the system clock frequency of 120 MHz.
  #if (120000000 % BOARD_XOSC_FREQ_HZ) != 0
    #error "BOARD_XOSC_FREQ_HZ must be an integer factor of 120 MHz"
  #endif
#endif

#if BOARD_XOSC_FREQ_HZ != 0
// External clock sources are currently not implemented for SAMD21 chips.
  #ifdef SAMD21
    #error "BOARD_XOSC_FREQ_HZ is non-zero but external clock sources are not yet supported for SAMD21 chips"
  #endif
  #ifndef BOARD_XOSC_IS_CRYSTAL
    #error "BOARD_XOSC_IS_CRYSTAL must be defined to 0 or 1 if BOARD_XOSC_FREQ_HZ is not 0"
  #endif
#else
// It doesn't matter what the value is in this case.
  #define BOARD_XOSC_IS_CRYSTAL (0)
#endif

// if CALIBRATE_CRYSTALLESS is requested, make room for storing
// calibration data generated from external USB.
#ifndef CIRCUITPY_INTERNAL_CONFIG_SIZE
  #if CALIBRATE_CRYSTALLESS
    #define CIRCUITPY_INTERNAL_CONFIG_SIZE (NVMCTRL_ROW_SIZE) // 256
  #else
    #define CIRCUITPY_INTERNAL_CONFIG_SIZE (0)
  #endif
#endif

// Flash layout, starting at 0x00000000
//
// - bootloader (8 or 16kB)
// - firmware
// - internal CIRCUITPY flash filesystem (optional)
// - internal config, used to store crystalless clock calibration info (optional)
// - microcontroller.nvm (optional)

// Define these regions starting up from the bottom of flash:

#define BOOTLOADER_START_ADDR          (0x00000000)

#define CIRCUITPY_FIRMWARE_START_ADDR  (BOOTLOADER_START_ADDR + BOOTLOADER_SIZE)

// Define these regions start down from the top of flash:

#define CIRCUITPY_INTERNAL_NVM_START_ADDR \
    (FLASH_SIZE - CIRCUITPY_INTERNAL_NVM_SIZE)

#define CIRCUITPY_INTERNAL_CONFIG_START_ADDR \
    (CIRCUITPY_INTERNAL_NVM_START_ADDR - CIRCUITPY_INTERNAL_CONFIG_SIZE)

#define CIRCUITPY_INTERNAL_FLASH_FILESYSTEM_START_ADDR \
    (CIRCUITPY_INTERNAL_CONFIG_START_ADDR - CIRCUITPY_INTERNAL_FLASH_FILESYSTEM_SIZE)

// The firmware space is the space left over between the fixed lower and upper regions.
#define CIRCUITPY_FIRMWARE_SIZE \
    (CIRCUITPY_INTERNAL_FLASH_FILESYSTEM_START_ADDR - CIRCUITPY_FIRMWARE_START_ADDR)

#if BOOTLOADER_START_ADDR % FLASH_PAGE_SIZE != 0
#error BOOTLOADER_START_ADDR must be on a flash page boundary.
#endif

#if CIRCUITPY_INTERNAL_NVM_START_ADDR % FLASH_ERASE_SIZE != 0
#error CIRCUITPY_INTERNAL_NVM_START_ADDR must be on a flash erase (row or block) boundary.
#endif
#if CIRCUITPY_INTERNAL_NVM_SIZE % FLASH_ERASE_SIZE != 0
#error CIRCUITPY_INTERNAL_NVM_SIZE must be a multiple of FLASH_ERASE_SIZE.
#endif

#if CIRCUITPY_INTERNAL_CONFIG_START_ADDR % FLASH_ERASE_SIZE != 0
#error CIRCUITPY_INTERNAL_CONFIG_SIZE must be on a flash erase (row or block) boundary.
#endif
#if CIRCUITPY_INTERNAL_CONFIG_SIZE % FLASH_ERASE_SIZE != 0
#error CIRCUITPY_INTERNAL_CONFIG_SIZE must be a multiple of FLASH_ERASE_SIZE.
#endif

#if CIRCUITPY_INTERNAL_FLASH_FILESYSTEM_START_ADDR % FLASH_ERASE_SIZE != 0
#error CIRCUITPY_INTERNAL_FLASH_FILESYSTEM_SIZE must be on a flash erase (row or block) boundary.
#endif
#if CIRCUITPY_INTERNAL_FLASH_FILESYSTEM_SIZE % FLASH_ERASE_SIZE != 0
#error CIRCUITPY_INTERNAL_FLASH_FILESYSTEM_SIZE must be a multiple of FLASH_ERASE_SIZE.
#endif

#if CIRCUITPY_FIRMWARE_SIZE < 0
#error No space left in flash for firmware after specifying other regions!
#endif

// Turning off audioio, audiobusio, and touchio as necessary
// due to limitations of chips is handled in mpconfigboard.mk

#include "peripherals/samd/dma.h"
