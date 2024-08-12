// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#include "py/obj.h"

#include "components/esp_event/include/esp_event.h"

#include "shared-bindings/wifi/ScannedNetworks.h"
#include "shared-bindings/wifi/Network.h"

#include "esp_netif_types.h"

// Event bits for the Radio event group.
#define WIFI_SCAN_DONE_BIT BIT0
#define WIFI_CONNECTED_BIT BIT1
#define WIFI_DISCONNECTED_BIT BIT2

typedef struct {
    mp_obj_base_t base;
    esp_event_handler_instance_t handler_instance_all_wifi;
    esp_event_handler_instance_t handler_instance_got_ip;
    wifi_scannednetworks_obj_t *current_scan;
    StaticEventGroup_t event_group;
    EventGroupHandle_t event_group_handle;
    wifi_config_t sta_config;
    wifi_network_obj_t ap_info;
    esp_netif_ip_info_t ip_info;
    esp_netif_dns_info_t dns_info;
    esp_netif_t *netif;
    uint32_t ping_elapsed_time;
    wifi_config_t ap_config;
    esp_netif_ip_info_t ap_ip_info;
    esp_netif_t *ap_netif;
    bool started;
    bool ap_mode;
    bool sta_mode;
    uint8_t retries_left;
    uint8_t starting_retries;
    uint8_t last_disconnect_reason;
} wifi_radio_obj_t;

extern void common_hal_wifi_radio_gc_collect(wifi_radio_obj_t *self);
