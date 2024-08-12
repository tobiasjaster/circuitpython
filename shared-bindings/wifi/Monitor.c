// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 microDev
//
// SPDX-License-Identifier: MIT

#include "py/mpstate.h"
#include "py/runtime.h"
#include "py/objproperty.h"

#include "shared-bindings/util.h"
#include "shared-bindings/wifi/Packet.h"
#include "shared-bindings/wifi/Monitor.h"

//| class Monitor:
//|     """For monitoring WiFi packets."""
//|

//|     def __init__(self, channel: Optional[int] = 1, queue: Optional[int] = 128) -> None:
//|         """Initialize `wifi.Monitor` singleton.
//|
//|         :param int channel: The WiFi channel to scan.
//|         :param int queue: The queue size for buffering the packet.
//|
//|         """
//|         ...
static mp_obj_t wifi_monitor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_channel, ARG_queue };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_channel, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = 1} },
        { MP_QSTR_queue, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = 128} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_int_t channel = mp_arg_validate_int_range(args[ARG_channel].u_int, 1, 13, MP_QSTR_channel);
    mp_int_t queue = mp_arg_validate_int_min(args[ARG_queue].u_int, 0, MP_QSTR_queue);

    wifi_monitor_obj_t *self = MP_STATE_VM(wifi_monitor_singleton);
    if (common_hal_wifi_monitor_deinited()) {
        self = mp_obj_malloc(wifi_monitor_obj_t, &wifi_monitor_type);
        common_hal_wifi_monitor_construct(self, channel, queue);
        MP_STATE_VM(wifi_monitor_singleton) = self;
    }

    return MP_OBJ_FROM_PTR(self);
}

//|     channel: int
//|     """The WiFi channel to scan."""
static mp_obj_t wifi_monitor_obj_get_channel(mp_obj_t self_in) {
    return common_hal_wifi_monitor_get_channel(self_in);
}
MP_DEFINE_CONST_FUN_OBJ_1(wifi_monitor_get_channel_obj, wifi_monitor_obj_get_channel);

static mp_obj_t wifi_monitor_obj_set_channel(mp_obj_t self_in, mp_obj_t channel) {
    mp_int_t c = mp_obj_get_int(channel);
    if (c < 1 || c > 13) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("%q out of bounds"), MP_QSTR_channel);
    }
    common_hal_wifi_monitor_set_channel(self_in, c);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(wifi_monitor_set_channel_obj, wifi_monitor_obj_set_channel);

MP_PROPERTY_GETSET(wifi_monitor_channel_obj,
    (mp_obj_t)&wifi_monitor_get_channel_obj,
    (mp_obj_t)&wifi_monitor_set_channel_obj);

//|     queue: int
//|     """The queue size for buffering the packet."""
static mp_obj_t wifi_monitor_obj_get_queue(mp_obj_t self_in) {
    return common_hal_wifi_monitor_get_queue(self_in);
}
MP_DEFINE_CONST_FUN_OBJ_1(wifi_monitor_get_queue_obj, wifi_monitor_obj_get_queue);

MP_PROPERTY_GETTER(wifi_monitor_queue_obj,
    (mp_obj_t)&wifi_monitor_get_queue_obj);

//|     def deinit(self) -> None:
//|         """De-initialize `wifi.Monitor` singleton."""
//|         ...
static mp_obj_t wifi_monitor_obj_deinit(mp_obj_t self_in) {
    common_hal_wifi_monitor_deinit(self_in);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(wifi_monitor_deinit_obj, wifi_monitor_obj_deinit);

//|     def lost(self) -> int:
//|         """Returns the packet loss count. The counter resets after each poll."""
//|         ...
static mp_obj_t wifi_monitor_obj_get_lost(mp_obj_t self_in) {
    return common_hal_wifi_monitor_get_lost(self_in);
}
MP_DEFINE_CONST_FUN_OBJ_1(wifi_monitor_lost_obj, wifi_monitor_obj_get_lost);

//|     def queued(self) -> int:
//|         """Returns the packet queued count."""
//|         ...
static mp_obj_t wifi_monitor_obj_get_queued(mp_obj_t self_in) {
    if (common_hal_wifi_monitor_deinited()) {
        return mp_obj_new_int_from_uint(0);
    }
    return common_hal_wifi_monitor_get_queued(self_in);
}
MP_DEFINE_CONST_FUN_OBJ_1(wifi_monitor_queued_obj, wifi_monitor_obj_get_queued);

//|     def packet(self) -> dict:
//|         """Returns the monitor packet."""
//|         ...
//|
static mp_obj_t wifi_monitor_obj_get_packet(mp_obj_t self_in) {
    if (common_hal_wifi_monitor_deinited()) {
        raise_deinited_error();
    }
    return common_hal_wifi_monitor_get_packet(self_in);
}
MP_DEFINE_CONST_FUN_OBJ_1(wifi_monitor_packet_obj, wifi_monitor_obj_get_packet);

static const mp_rom_map_elem_t wifi_monitor_locals_dict_table[] = {
    // properties
    { MP_ROM_QSTR(MP_QSTR_channel), MP_ROM_PTR(&wifi_monitor_channel_obj) },
    { MP_ROM_QSTR(MP_QSTR_queue),   MP_ROM_PTR(&wifi_monitor_queue_obj) },

    // functions
    { MP_ROM_QSTR(MP_QSTR_deinit),  MP_ROM_PTR(&wifi_monitor_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_lost),    MP_ROM_PTR(&wifi_monitor_lost_obj) },
    { MP_ROM_QSTR(MP_QSTR_queued),  MP_ROM_PTR(&wifi_monitor_queued_obj) },
    { MP_ROM_QSTR(MP_QSTR_packet),  MP_ROM_PTR(&wifi_monitor_packet_obj) },
};
static MP_DEFINE_CONST_DICT(wifi_monitor_locals_dict, wifi_monitor_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    wifi_monitor_type,
    MP_QSTR_Monitor,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    make_new, wifi_monitor_make_new,
    locals_dict, &wifi_monitor_locals_dict
    );

MP_REGISTER_ROOT_POINTER(mp_obj_t wifi_monitor_singleton);
