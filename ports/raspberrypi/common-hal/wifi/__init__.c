// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "common-hal/wifi/__init__.h"
#include "shared-bindings/wifi/__init__.h"

#include "shared-bindings/ipaddress/IPv4Address.h"
#include "shared-bindings/wifi/Monitor.h"
#include "shared-bindings/wifi/Radio.h"

#include "py/mperrno.h"
#include "py/mpstate.h"
#include "py/runtime.h"

wifi_radio_obj_t common_hal_wifi_radio_obj;

#include "supervisor/port.h"
#include "supervisor/shared/status_bar.h"
#include "supervisor/workflow.h"

static bool wifi_inited;
static bool wifi_ever_inited;
static bool wifi_user_initiated;

void common_hal_wifi_init(bool user_initiated) {
    wifi_radio_obj_t *self = &common_hal_wifi_radio_obj;

    if (wifi_inited) {
        if (user_initiated && !wifi_user_initiated) {
            common_hal_wifi_radio_set_enabled(self, true);
        }
        return;
    }
    wifi_inited = true;
    wifi_user_initiated = user_initiated;
    common_hal_wifi_radio_obj.base.type = &wifi_radio_type;
    common_hal_wifi_radio_obj.current_scan = NULL;

    if (!wifi_ever_inited) {
    }
    wifi_ever_inited = true;

    // set station mode to avoid the default SoftAP
    common_hal_wifi_radio_start_station(self);
    // start wifi
    common_hal_wifi_radio_set_enabled(self, true);
}

void wifi_user_reset(void) {
    if (wifi_user_initiated) {
        wifi_reset();
        wifi_user_initiated = false;
    }
}

void wifi_reset(void) {
    if (!wifi_inited) {
        return;
    }
    // the cyw43 wifi chip is not reset due to https://github.com/raspberrypi/pico-sdk/issues/980
    common_hal_wifi_monitor_deinit(MP_STATE_VM(wifi_monitor_singleton));
    common_hal_wifi_radio_obj.current_scan = NULL;
    common_hal_wifi_radio_set_enabled(&common_hal_wifi_radio_obj, false);
    supervisor_workflow_request_background();
}

void common_hal_wifi_gc_collect(void) {
    common_hal_wifi_radio_gc_collect(&common_hal_wifi_radio_obj);
}

void raise_cyw_error(int err) {
    int mp_errno;
    switch (err) {
        case -CYW43_EIO:
            mp_errno = MP_EIO;
            break;
        case -CYW43_EPERM:
            mp_errno = MP_EPERM;
            break;
        case -CYW43_EINVAL:
            mp_errno = MP_EINVAL;
            break;
        case -CYW43_ETIMEDOUT:
            mp_errno = MP_ETIMEDOUT;
            break;
        default:
            mp_raise_OSError_msg_varg(MP_ERROR_TEXT("Unknown error code %d"), err);
    }
    mp_raise_OSError(mp_errno);
}

void ipaddress_ipaddress_to_lwip(mp_obj_t ip_address, ip_addr_t *lwip_ip_address) {
    if (!mp_obj_is_type(ip_address, &ipaddress_ipv4address_type)) {
        mp_raise_ValueError(MP_ERROR_TEXT("Only IPv4 addresses supported"));
    }
    mp_obj_t packed = common_hal_ipaddress_ipv4address_get_packed(ip_address);
    size_t len;
    const char *bytes = mp_obj_str_get_data(packed, &len);

    IP_ADDR4(lwip_ip_address, bytes[0], bytes[1], bytes[2], bytes[3]);
}
