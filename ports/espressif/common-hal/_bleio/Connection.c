// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2018 Dan Halbert for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2018 Artur Pacholec
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/_bleio/Connection.h"

#include <string.h>
#include <stdio.h>

#include "py/gc.h"
#include "py/objlist.h"
#include "py/objstr.h"
#include "py/qstr.h"
#include "py/runtime.h"

#include "shared/runtime/interrupt_char.h"

#include "shared-bindings/_bleio/__init__.h"
#include "shared-bindings/_bleio/Adapter.h"
#include "shared-bindings/_bleio/Attribute.h"
#include "shared-bindings/_bleio/Characteristic.h"
#include "shared-bindings/_bleio/Service.h"
#include "shared-bindings/_bleio/UUID.h"
#include "shared-bindings/time/__init__.h"

#include "supervisor/shared/tick.h"

#include "common-hal/_bleio/ble_events.h"

#include "host/ble_att.h"

// Uncomment to turn on debug logging just in this file.
// #undef CIRCUITPY_VERBOSE_BLE
// #define CIRCUITPY_VERBOSE_BLE (1)

int bleio_connection_event_cb(struct ble_gap_event *event, void *connection_in) {
    bleio_connection_internal_t *connection = (bleio_connection_internal_t *)connection_in;

    switch (event->type) {
        case BLE_GAP_EVENT_DISCONNECT: {
            connection->conn_handle = BLEIO_HANDLE_INVALID;
            connection->pair_status = PAIR_NOT_PAIRED;

            #if CIRCUITPY_VERBOSE_BLE
            mp_printf(&mp_plat_print, "event->disconnect.reason: 0x%x\n", event->disconnect.reason);
            #endif

            if (connection->connection_obj != mp_const_none) {
                bleio_connection_obj_t *obj = connection->connection_obj;
                obj->connection = NULL;
                obj->disconnect_reason = event->disconnect.reason;
            }

            break;
        }

        case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE: {
            // Nothing to do here. CircuitPython doesn't tell the user what PHY
            // we're on.
            break;
        }

        case BLE_GAP_EVENT_CONN_UPDATE: {
            struct ble_gap_conn_desc desc;
            int rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
            assert(rc == 0);
            connection->conn_params_updating = false;
            break;
        }
        case BLE_GAP_EVENT_ENC_CHANGE: {
            struct ble_gap_conn_desc desc;
            ble_gap_conn_find(event->enc_change.conn_handle, &desc);
            if (desc.sec_state.encrypted) {
                connection->pair_status = PAIR_PAIRED;
            }
            break;
        }
        case BLE_GAP_EVENT_MTU: {
            if (event->mtu.conn_handle != connection->conn_handle) {
                return 0;
            }
            connection->mtu = event->mtu.value;
            break;
        }

        // These events are actually att specific so forward to all registered
        // handlers for them. The handlers themselves decide whether an event
        // is interesting to them.
        case BLE_GAP_EVENT_NOTIFY_RX:
            MP_FALLTHROUGH;
        case BLE_GAP_EVENT_NOTIFY_TX:
            MP_FALLTHROUGH;
        case BLE_GAP_EVENT_SUBSCRIBE:
            int status = ble_event_run_handlers(event);
            background_callback_add_core(&bleio_background_callback);
            return status;

        default:
            #if CIRCUITPY_VERBOSE_BLE
            mp_printf(&mp_plat_print, "Unhandled connection event: %d\n", event->type);
            #endif
            break;
    }

    background_callback_add_core(&bleio_background_callback);
    return 0;
}

bool common_hal_bleio_connection_get_paired(bleio_connection_obj_t *self) {
    if (self->connection == NULL) {
        return false;
    }
    return self->connection->pair_status == PAIR_PAIRED;
}

bool common_hal_bleio_connection_get_connected(bleio_connection_obj_t *self) {
    if (self->connection == NULL) {
        return false;
    }
    return self->connection->conn_handle != BLEIO_HANDLE_INVALID;
}

void common_hal_bleio_connection_disconnect(bleio_connection_internal_t *self) {
    // Second argument is an HCI reason, not an HS error code.
    ble_gap_terminate(self->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
}

void common_hal_bleio_connection_pair(bleio_connection_internal_t *self, bool bond) {
    // We may already be trying to pair if we just reconnected to a peer we're
    // bonded with.
    while (self->pair_status == PAIR_WAITING && !mp_hal_is_interrupted()) {
        RUN_BACKGROUND_TASKS;
    }
    if (self->pair_status == PAIR_PAIRED) {
        return;
    }
    self->pair_status = PAIR_WAITING;
    CHECK_NIMBLE_ERROR(ble_gap_security_initiate(self->conn_handle));
    while (self->pair_status == PAIR_WAITING && !mp_hal_is_interrupted()) {
        RUN_BACKGROUND_TASKS;
    }
    if (mp_hal_is_interrupted()) {
        return;
    }
}

mp_float_t common_hal_bleio_connection_get_connection_interval(bleio_connection_internal_t *self) {
    while (self->conn_params_updating && !mp_hal_is_interrupted()) {
        RUN_BACKGROUND_TASKS;
    }
    if (mp_hal_is_interrupted()) {
        return 0;
    }
    struct ble_gap_conn_desc desc;
    CHECK_NIMBLE_ERROR(ble_gap_conn_find(self->conn_handle, &desc));
    return 1.25f * desc.conn_itvl;
}

// Return the current negotiated MTU length, minus overhead.
mp_int_t common_hal_bleio_connection_get_max_packet_length(bleio_connection_internal_t *self) {
    return (self->mtu == 0 ? BLE_ATT_MTU_DFLT : self->mtu) - 3;
}

void common_hal_bleio_connection_set_connection_interval(bleio_connection_internal_t *self, mp_float_t new_interval) {
    self->conn_params_updating = true;
    struct ble_gap_conn_desc desc;
    CHECK_NIMBLE_ERROR(ble_gap_conn_find(self->conn_handle, &desc));
    uint16_t interval = new_interval / 1.25f;
    struct ble_gap_upd_params updated = {
        .itvl_min = interval,
        .itvl_max = interval,
        .latency = desc.conn_latency,
        .supervision_timeout = desc.supervision_timeout
    };
    CHECK_NIMBLE_ERROR(ble_gap_update_params(self->conn_handle, &updated));
}

// Zero when discovery is in process. BLE_HS_EDONE or a BLE_HS_ error code when done.
static volatile int _last_discovery_status;

static uint64_t _discovery_start_time;

// Give 20 seconds for discovery
#define DISCOVERY_TIMEOUT_MS 20000

static void _start_discovery_timeout(void) {
    _discovery_start_time = common_hal_time_monotonic_ms();
}

static int _wait_for_discovery_step_done(void) {
    const uint64_t timeout_time_ms = _discovery_start_time + DISCOVERY_TIMEOUT_MS;
    while ((_last_discovery_status == 0) && (common_hal_time_monotonic_ms() < timeout_time_ms)) {
        RUN_BACKGROUND_TASKS;
        if (mp_hal_is_interrupted()) {
            // Return prematurely. Then the interrupt will be raised.
            _last_discovery_status = BLE_HS_EDONE;
        }
    }
    return _last_discovery_status;
}

// Record result of last discovery step: services, characteristics, descriptors.
static void _set_discovery_step_status(int status) {
    _last_discovery_status = status;
}

static int _discovered_service_cb(uint16_t conn_handle,
    const struct ble_gatt_error *error,
    const struct ble_gatt_svc *svc,
    void *arg) {
    bleio_connection_internal_t *self = (bleio_connection_internal_t *)arg;

    if (error->status != 0) {
        // BLE_HS_EDONE or some error has occurred.
        _set_discovery_step_status(error->status);
        return 0;
    }

    bleio_service_obj_t *service = mp_obj_malloc(bleio_service_obj_t, &bleio_service_type);

    // Initialize several fields at once.
    bleio_service_from_connection(service, bleio_connection_new_from_internal(self));

    service->is_remote = true;
    service->start_handle = svc->start_handle;
    service->end_handle = svc->end_handle;
    service->handle = svc->start_handle;

    bleio_uuid_obj_t *uuid = mp_obj_malloc(bleio_uuid_obj_t, &bleio_uuid_type);

    uuid->nimble_ble_uuid = svc->uuid;
    service->uuid = uuid;

    mp_obj_list_append(MP_OBJ_FROM_PTR(self->remote_service_list),
        MP_OBJ_FROM_PTR(service));
    return 0;
}

static int _discovered_characteristic_cb(uint16_t conn_handle,
    const struct ble_gatt_error *error,
    const struct ble_gatt_chr *chr,
    void *arg) {
    bleio_service_obj_t *service = (bleio_service_obj_t *)arg;

    if (error->status != 0) {
        // BLE_HS_EDONE or some error has occurred.
        _set_discovery_step_status(error->status);
        return 0;
    }

    bleio_characteristic_obj_t *characteristic =
        mp_obj_malloc(bleio_characteristic_obj_t, &bleio_characteristic_type);

    // Known characteristic UUID.
    bleio_uuid_obj_t *uuid = mp_obj_malloc(bleio_uuid_obj_t, &bleio_uuid_type);

    uuid->nimble_ble_uuid = chr->uuid;

    bleio_characteristic_properties_t props =
        ((chr->properties & BLE_GATT_CHR_PROP_BROADCAST) != 0 ? CHAR_PROP_BROADCAST : 0) |
        ((chr->properties & BLE_GATT_CHR_PROP_INDICATE) != 0 ? CHAR_PROP_INDICATE : 0) |
        ((chr->properties & BLE_GATT_CHR_PROP_NOTIFY) != 0 ? CHAR_PROP_NOTIFY : 0) |
        ((chr->properties & BLE_GATT_CHR_PROP_READ) != 0 ? CHAR_PROP_READ : 0) |
        ((chr->properties & BLE_GATT_CHR_PROP_WRITE) != 0 ? CHAR_PROP_WRITE : 0) |
        ((chr->properties & BLE_GATT_CHR_PROP_WRITE_NO_RSP) != 0 ? CHAR_PROP_WRITE_NO_RESPONSE : 0);

    // Call common_hal_bleio_characteristic_construct() to initialize some fields and set up evt handler.
    mp_buffer_info_t mp_const_empty_bytes_bufinfo;
    mp_get_buffer_raise(mp_const_empty_bytes, &mp_const_empty_bytes_bufinfo, MP_BUFFER_READ);

    common_hal_bleio_characteristic_construct(
        characteristic, service, chr->val_handle, uuid,
        props, SECURITY_MODE_OPEN, SECURITY_MODE_OPEN,
        GATT_MAX_DATA_LENGTH, false,   // max_length, fixed_length: values don't matter for gattc, but don't use 0
        &mp_const_empty_bytes_bufinfo,
        NULL);
    // Set def_handle directly since it is only used in discovery.
    characteristic->def_handle = chr->def_handle;

    #if CIRCUITPY_VERBOSE_BLE
    mp_printf(&mp_plat_print, "_discovered_characteristic_cb: char handle: %d\n", characteristic->handle);
    #endif

    mp_obj_list_append(MP_OBJ_FROM_PTR(service->characteristic_list),
        MP_OBJ_FROM_PTR(characteristic));
    return 0;
}

static int _discovered_descriptor_cb(uint16_t conn_handle,
    const struct ble_gatt_error *error,
    uint16_t chr_val_handle,
    const struct ble_gatt_dsc *dsc,
    void *arg) {
    bleio_characteristic_obj_t *characteristic = (bleio_characteristic_obj_t *)arg;

    if (error->status != 0) {

        #if CIRCUITPY_VERBOSE_BLE
        mp_printf(&mp_plat_print, "_discovered_descriptor_cb error->status: %d, handle: %d\n",
            error->status, error->att_handle);
        #endif

        // BLE_HS_EDONE or some error has occurred.
        _set_discovery_step_status(error->status);
        return 0;
    }

    // Remember handles for certain well-known descriptors.
    switch (dsc->uuid.u16.value) {
        case 0x2902:
            characteristic->cccd_handle = dsc->handle;
            break;

        case 0x2903:
            characteristic->sccd_handle = dsc->handle;
            break;

        case 0x2901:
            characteristic->user_desc_handle = dsc->handle;
            break;

        default:
            break;
    }

    bleio_descriptor_obj_t *descriptor = mp_obj_malloc(bleio_descriptor_obj_t, &bleio_descriptor_type);

    bleio_uuid_obj_t *uuid = mp_obj_malloc(bleio_uuid_obj_t, &bleio_uuid_type);
    uuid->nimble_ble_uuid = dsc->uuid;
    common_hal_bleio_descriptor_construct(
        descriptor, characteristic, uuid,
        SECURITY_MODE_OPEN, SECURITY_MODE_OPEN,
        GATT_MAX_DATA_LENGTH, false, mp_const_empty_bytes);
    descriptor->handle = dsc->handle;

    #if CIRCUITPY_VERBOSE_BLE
    mp_printf(&mp_plat_print, "_discovered_descriptor_cb: char handle: %d, desc handle: %d, uuid type: %d, u16 value: 0x%x\n",
        characteristic->handle, descriptor->handle, dsc->uuid.u.type, dsc->uuid.u16.value);
    #endif

    mp_obj_list_append(MP_OBJ_FROM_PTR(characteristic->descriptor_list),
        MP_OBJ_FROM_PTR(descriptor));
    return 0;
}

static void discover_remote_services(bleio_connection_internal_t *self, mp_obj_t service_uuids_whitelist) {
    // Start over with an empty list.
    self->remote_service_list = mp_obj_new_list(0, NULL);

    // Start timeout in case discovery gets stuck.
    _start_discovery_timeout();

    if (service_uuids_whitelist == mp_const_none) {
        // Reset discovery status before starting callbacks
        _set_discovery_step_status(0);

        CHECK_NIMBLE_ERROR(ble_gattc_disc_all_svcs(self->conn_handle, _discovered_service_cb, self));

        // Wait for _discovered_service_cb() to be called multiple times until it's done.
        int status = _wait_for_discovery_step_done();
        if (status != BLE_HS_EDONE) {
            CHECK_BLE_ERROR(status);
        }
    } else {
        mp_obj_iter_buf_t iter_buf;
        mp_obj_t iterable = mp_getiter(service_uuids_whitelist, &iter_buf);
        mp_obj_t uuid_obj;
        while ((uuid_obj = mp_iternext(iterable)) != MP_OBJ_STOP_ITERATION) {
            if (!mp_obj_is_type(uuid_obj, &bleio_uuid_type)) {
                mp_raise_TypeError(MP_ERROR_TEXT("non-UUID found in service_uuids_whitelist"));
            }
            bleio_uuid_obj_t *uuid = MP_OBJ_TO_PTR(uuid_obj);

            // Reset discovery status before starting callbacks
            _set_discovery_step_status(0);

            CHECK_NIMBLE_ERROR(ble_gattc_disc_svc_by_uuid(self->conn_handle, &uuid->nimble_ble_uuid.u,
                _discovered_service_cb, self));

            // Wait for _discovered_service_cb() to be called multiple times until it's done.
            int status = _wait_for_discovery_step_done();
            if (status != BLE_HS_EDONE) {
                CHECK_BLE_ERROR(status);
            }
        }
    }

    // Now discover characteristics for each discovered service.

    for (size_t i = 0; i < self->remote_service_list->len; i++) {
        bleio_service_obj_t *service = MP_OBJ_TO_PTR(self->remote_service_list->items[i]);

        // Reset discovery status before starting callbacks
        _set_discovery_step_status(0);

        CHECK_NIMBLE_ERROR(ble_gattc_disc_all_chrs(self->conn_handle,
            service->start_handle,
            service->end_handle,
            _discovered_characteristic_cb,
            service));

        // Wait for _discovered_characteristic_cb() to be called multiple times until it's done.
        int status = _wait_for_discovery_step_done();
        if (status != BLE_HS_EDONE) {
            CHECK_BLE_ERROR(status);
        }

        // Got characteristics for this service. Now discover descriptors for each characteristic.
        size_t char_list_len = service->characteristic_list->len;
        for (size_t char_idx = 0; char_idx < char_list_len; ++char_idx) {
            bleio_characteristic_obj_t *characteristic =
                MP_OBJ_TO_PTR(service->characteristic_list->items[char_idx]);
            // Determine the handle range for the given characteristic's descriptors.
            // The end of the range is dictated by the next characteristic or the end
            // handle of the service.
            const bool last_characteristic = char_idx == char_list_len - 1;
            bleio_characteristic_obj_t *next_characteristic = last_characteristic
                ? NULL
                : MP_OBJ_TO_PTR(service->characteristic_list->items[char_idx + 1]);

            uint16_t end_handle = next_characteristic == NULL
                ? service->end_handle
                : next_characteristic->handle - 1;

            // Pre-check if there are no descriptors to discover so descriptor discovery doesn't fail
            if (end_handle <= characteristic->handle) {
                continue;
            }

            // Reset discovery status before starting callbacks
            _set_discovery_step_status(0);

            // The descriptor handle inclusive range is [characteristic->handle + 1, end_handle],
            // but ble_gattc_disc_all_dscs() requires starting with characteristic->handle.
            CHECK_NIMBLE_ERROR(ble_gattc_disc_all_dscs(self->conn_handle, characteristic->handle,
                end_handle,
                _discovered_descriptor_cb, characteristic));

            // Wait for _discovered_descriptor_cb to be called multiple times until it's done.
            status = _wait_for_discovery_step_done();
            if (status != BLE_HS_EDONE) {
                CHECK_BLE_ERROR(status);
            }
        }
    }
}

mp_obj_tuple_t *common_hal_bleio_connection_discover_remote_services(bleio_connection_obj_t *self, mp_obj_t service_uuids_whitelist) {
    discover_remote_services(self->connection, service_uuids_whitelist);
    bleio_connection_ensure_connected(self);
    // Convert to a tuple and then clear the list so the callee will take ownership.
    mp_obj_tuple_t *services_tuple =
        mp_obj_new_tuple(self->connection->remote_service_list->len,
            self->connection->remote_service_list->items);
    mp_obj_list_clear(MP_OBJ_FROM_PTR(self->connection->remote_service_list));

    return services_tuple;
}

uint16_t bleio_connection_get_conn_handle(bleio_connection_obj_t *self) {
    if (self == NULL || self->connection == NULL) {
        return BLEIO_HANDLE_INVALID;
    }
    return self->connection->conn_handle;
}

mp_obj_t bleio_connection_new_from_internal(bleio_connection_internal_t *internal) {
    if (internal->connection_obj != mp_const_none) {
        return internal->connection_obj;
    }
    bleio_connection_obj_t *connection = mp_obj_malloc(bleio_connection_obj_t, &bleio_connection_type);

    connection->connection = internal;
    internal->connection_obj = connection;

    return MP_OBJ_FROM_PTR(connection);
}

// Find the connection that uses the given conn_handle. Return NULL if not found.
bleio_connection_internal_t *bleio_conn_handle_to_connection(uint16_t conn_handle) {
    bleio_connection_internal_t *connection;
    for (size_t i = 0; i < BLEIO_TOTAL_CONNECTION_COUNT; i++) {
        connection = &bleio_connections[i];
        if (connection->conn_handle == conn_handle) {
            return connection;
        }
    }

    return NULL;
}
