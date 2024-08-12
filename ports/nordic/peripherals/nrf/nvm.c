// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2013, 2014 Damien P. George
// SPDX-FileCopyrightText: Copyright (c) 2019 Nick Moore for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "py/runtime.h"

#include <stdio.h>
#include <string.h>

#include "nrfx_nvmc.h"

#define FLASH_PAGE_SIZE (4096)

#ifdef BLUETOOTH_SD
#include "ble_drv.h"
#include "nrf_sdm.h"

static bool sd_is_enabled(void) {
    uint8_t sd_en = 0;
    if (__get_PRIMASK()) {
        return false;
    }
    (void)sd_softdevice_is_enabled(&sd_en);
    return sd_en;
}

static void sd_flash_operation_start(void) {
    sd_flash_operation_status = SD_FLASH_OPERATION_IN_PROGRESS;
}

static sd_flash_operation_status_t sd_flash_operation_wait_until_done(void) {
    // If the SD is not enabled, no events are generated, so just return immediately.
    if (sd_is_enabled()) {
        while (sd_flash_operation_status == SD_FLASH_OPERATION_IN_PROGRESS) {
            sd_app_evt_wait();
        }
    } else {
        sd_flash_operation_status = SD_FLASH_OPERATION_DONE;
    }
    return sd_flash_operation_status;

}

bool sd_flash_page_erase_sync(uint32_t page_number) {
    sd_flash_operation_start();
    if (sd_flash_page_erase(page_number) != NRF_SUCCESS) {
        return false;
    }
    if (sd_flash_operation_wait_until_done() == SD_FLASH_OPERATION_ERROR) {
        return false;
    }
    return true;
}

bool sd_flash_write_sync(uint32_t *dest_words, uint32_t *src_words, uint32_t num_words) {
    sd_flash_operation_start();
    if (sd_flash_write(dest_words, src_words, num_words) != NRF_SUCCESS) {
        return false;
    }
    if (sd_flash_operation_wait_until_done() == SD_FLASH_OPERATION_ERROR) {
        return false;
    }
    return true;
}

#endif

// The nRF52840 datasheet specifies a maximum of two writes to a flash
// location before an erase is necessary, even if the write is all
// ones (erased state).  So we can't avoid erases even if the page
// appears to be already erased (all ones), unless we keep track of
// writes to a page.

bool nrf_nvm_safe_flash_page_write(uint32_t page_addr, uint8_t *data) {
    #ifdef BLUETOOTH_SD
    if (sd_is_enabled()) {
        uint32_t err_code;
        sd_flash_operation_status_t status;

        sd_flash_operation_start();
        err_code = sd_flash_page_erase(page_addr / FLASH_PAGE_SIZE);
        if (err_code != NRF_SUCCESS) {
            return false;
        }
        status = sd_flash_operation_wait_until_done();
        if (status == SD_FLASH_OPERATION_ERROR) {
            return false;
        }

        // Divide a full page into parts, because writing a full page causes an assertion failure.
        // See https://devzone.nordicsemi.com/f/nordic-q-a/40088/sd_flash_write-cause-nrf_fault_id_sd_assert/
        const size_t BLOCK_PARTS = 2;
        size_t words_to_write = FLASH_PAGE_SIZE / sizeof(uint32_t) / BLOCK_PARTS;
        for (size_t i = 0; i < BLOCK_PARTS; i++) {
            sd_flash_operation_start();
            err_code = sd_flash_write(((uint32_t *)page_addr) + i * words_to_write,
                (uint32_t *)data + i * words_to_write,
                words_to_write);
            if (err_code != NRF_SUCCESS) {
                return false;
            }
            status = sd_flash_operation_wait_until_done();
            if (status == SD_FLASH_OPERATION_ERROR) {
                return false;
            }
        }

        return true;
    }
    #endif

    nrfx_nvmc_page_erase(page_addr);
    nrfx_nvmc_bytes_write(page_addr, data, FLASH_PAGE_SIZE);
    return true;
}
