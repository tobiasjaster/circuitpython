// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2024 Adafruit Industries LLC
//
// SPDX-License-Identifier: MIT

// Fake source file used only to capture #define values for use in ld template files.
#include "mpconfigport.h"

// For each value needed in the LD file, create a C-like line:
// /*NAME_OF_VALUE=*/ NAME_OF_VALUE;
// The C preprocessor will replace NAME_OF_VALUE with the actual value.
// This will be post-processed by tools/gen_ld_files.py to extract the name and value.

// The next line is a marker to start looking for definitions. Lines above the next line are ignored.
// START_LD_DEFINES

/*RAM_SIZE=*/ RAM_SIZE;
/*FLASH_SIZE=*/ FLASH_SIZE;

/*BOOTLOADER_SIZE=*/ BOOTLOADER_SIZE;
/*BOOTLOADER_START_ADDR=*/ BOOTLOADER_START_ADDR;

/*CIRCUITPY_DEFAULT_STACK_SIZE=*/ CIRCUITPY_DEFAULT_STACK_SIZE;

/*CIRCUITPY_FIRMWARE_START_ADDR=*/ CIRCUITPY_FIRMWARE_START_ADDR;
/*CIRCUITPY_FIRMWARE_SIZE=*/ CIRCUITPY_FIRMWARE_SIZE;

/*CIRCUITPY_INTERNAL_CONFIG_START_ADDR=*/ CIRCUITPY_INTERNAL_CONFIG_START_ADDR;
/*CIRCUITPY_INTERNAL_CONFIG_SIZE=*/ CIRCUITPY_INTERNAL_CONFIG_SIZE;

/*CIRCUITPY_INTERNAL_NVM_START_ADDR=*/ CIRCUITPY_INTERNAL_NVM_START_ADDR;
/*CIRCUITPY_INTERNAL_NVM_SIZE=*/ CIRCUITPY_INTERNAL_NVM_SIZE;

/*CIRCUITPY_INTERNAL_FLASH_FILESYSTEM_START_ADDR=*/ CIRCUITPY_INTERNAL_FLASH_FILESYSTEM_START_ADDR;
/*CIRCUITPY_INTERNAL_FLASH_FILESYSTEM_SIZE=*/ CIRCUITPY_INTERNAL_FLASH_FILESYSTEM_SIZE;
