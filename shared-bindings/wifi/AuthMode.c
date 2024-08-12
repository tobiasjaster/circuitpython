// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 microDev
//
// SPDX-License-Identifier: MIT

#include "py/enum.h"

#include "shared-bindings/wifi/AuthMode.h"

MAKE_ENUM_VALUE(wifi_authmode_type, authmode, OPEN, AUTHMODE_OPEN);
MAKE_ENUM_VALUE(wifi_authmode_type, authmode, WEP, AUTHMODE_WEP);
MAKE_ENUM_VALUE(wifi_authmode_type, authmode, WPA, AUTHMODE_WPA);
MAKE_ENUM_VALUE(wifi_authmode_type, authmode, WPA2, AUTHMODE_WPA2);
MAKE_ENUM_VALUE(wifi_authmode_type, authmode, WPA3, AUTHMODE_WPA3);
MAKE_ENUM_VALUE(wifi_authmode_type, authmode, PSK, AUTHMODE_PSK);
MAKE_ENUM_VALUE(wifi_authmode_type, authmode, ENTERPRISE, AUTHMODE_ENTERPRISE);

//| class AuthMode:
//|     """The authentication protocols used by WiFi."""
//|
//|     OPEN: object
//|     """Open network. No authentication required."""
//|
//|     WEP: object
//|     """Wired Equivalent Privacy."""
//|
//|     WPA: object
//|     """Wireless Protected Access."""
//|
//|     WPA2: object
//|     """Wireless Protected Access 2."""
//|
//|     WPA3: object
//|     """Wireless Protected Access 3."""
//|
//|     PSK: object
//|     """Pre-shared Key. (password)"""
//|
//|     ENTERPRISE: object
//|     """Each user has a unique credential."""
//|
MAKE_ENUM_MAP(wifi_authmode) {
    MAKE_ENUM_MAP_ENTRY(authmode, OPEN),
    MAKE_ENUM_MAP_ENTRY(authmode, WEP),
    MAKE_ENUM_MAP_ENTRY(authmode, WPA),
    MAKE_ENUM_MAP_ENTRY(authmode, WPA2),
    MAKE_ENUM_MAP_ENTRY(authmode, WPA3),
    MAKE_ENUM_MAP_ENTRY(authmode, PSK),
    MAKE_ENUM_MAP_ENTRY(authmode, ENTERPRISE),
};
static MP_DEFINE_CONST_DICT(wifi_authmode_locals_dict, wifi_authmode_locals_table);

MAKE_PRINTER(wifi, wifi_authmode);

MAKE_ENUM_TYPE(wifi, AuthMode, wifi_authmode);
