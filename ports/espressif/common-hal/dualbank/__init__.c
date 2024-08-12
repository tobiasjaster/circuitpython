// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 microDev
//
// SPDX-License-Identifier: MIT

#include "common-hal/dualbank/__init__.h"
#include "shared-bindings/dualbank/__init__.h"

#include <string.h>

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"

static const esp_partition_t *update_partition = NULL;
static esp_ota_handle_t update_handle = 0;

static const char *TAG = "dualbank";

void dualbank_reset(void) {
    if (esp_ota_abort(update_handle) == ESP_OK) {
        update_handle = 0;
        update_partition = NULL;
    }
}

static void __attribute__((noreturn)) task_fatal_error(void) {
    ESP_LOGE(TAG, "Exiting task due to fatal error...");
    mp_raise_RuntimeError(MP_ERROR_TEXT("Update failed"));
}

void common_hal_dualbank_flash(const void *buf, const size_t len, const size_t offset) {
    esp_err_t err;

    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *last_invalid = esp_ota_get_last_invalid_partition();

    if (update_partition == NULL) {
        update_partition = esp_ota_get_next_update_partition(NULL);
        assert(update_partition != NULL);

        ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08lu)",
            running->type, running->subtype, running->address);

        ESP_LOGI(TAG, "Writing partition type %d subtype %d (offset 0x%08lu)\n",
            update_partition->type, update_partition->subtype, update_partition->address);
    }

    if (update_handle == 0) {
        if (len > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
            esp_app_desc_t new_app_info;
            memcpy(&new_app_info, &((char *)buf)[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
            ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

            esp_app_desc_t running_app_info;
            if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
                ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
            }

            esp_app_desc_t invalid_app_info;
            if (esp_ota_get_partition_description(last_invalid, &invalid_app_info) == ESP_OK) {
                ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
            }

            // check new version with running version
            if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
                ESP_LOGW(TAG, "New version is the same as running version.");
                mp_raise_RuntimeError(MP_ERROR_TEXT("Firmware is duplicate"));
            }

            // check new version with last invalid partition
            if (last_invalid != NULL) {
                if (memcmp(new_app_info.version, invalid_app_info.version, sizeof(new_app_info.version)) == 0) {
                    ESP_LOGW(TAG, "New version is the same as invalid version.");
                    mp_raise_RuntimeError(MP_ERROR_TEXT("Firmware is invalid"));
                }
            }

            err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                task_fatal_error();
            }
        } else {
            ESP_LOGE(TAG, "received package is not fit len");
            mp_raise_RuntimeError(MP_ERROR_TEXT("Firmware is too big"));
        }
    }

    if (offset == 0) {
        err = esp_ota_write(update_handle, buf, len);
    } else {
        err = esp_ota_write_with_offset(update_handle, buf, len, offset);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
        task_fatal_error();
    }
}

void common_hal_dualbank_switch(void) {
    if (esp_ota_end(update_handle) == ESP_OK) {
        update_handle = 0;
        update_partition = NULL;
    }
    esp_err_t err = esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL));
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
            mp_raise_RuntimeError(MP_ERROR_TEXT("Firmware is invalid"));
        }
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        task_fatal_error();
    }
}
