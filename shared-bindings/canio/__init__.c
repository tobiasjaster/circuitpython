// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Jeff Epler for Adafruit Industries
//
// SPDX-License-Identifier: MIT

//| """CAN bus access
//|
//| The `canio` module contains low level classes to support the CAN bus
//| protocol on microcontrollers that have built-in CAN peripherals.
//|
//| Boards like the Adafruit RP2040 CAN Bus Feather that use an MCP2515 or
//| compatible chip use the `mcp2515:adafruit_mcp2515` module instead.
//|
//| CAN and Listener classes change hardware state and should be deinitialized when they
//| are no longer needed if the program continues after use. To do so, either
//| call :py:meth:`!deinit` or use a context manager. See
//| :ref:`lifetime-and-contextmanagers` for more info.
//|
//| For example::
//|
//|   import canio
//|   from board import *
//|
//|   can = canio.CAN(board.CAN_RX, board.CAN_TX, baudrate=1000000)
//|   message = canio.Message(id=0x0408, data=b"adafruit")
//|   can.send(message)
//|   can.deinit()
//|
//| This example will write the data 'adafruit' onto the CAN bus to any
//| device listening for message id 0x0408.
//|
//| A CAN bus involves a transceiver, which is often a separate chip with a "standby" pin.
//| If your board has a CAN_STANDBY pin, ensure to set it to an output with the value False
//| to enable the transceiver.
//|
//| Other implementations of the CAN device may exist (for instance, attached
//| via an SPI bus).  If so their constructor arguments may differ, but
//| otherwise we encourage implementors to follow the API that the core uses.
//|
//| For more information on working with this module, refer to
//| `this Learn Guide on using it <https://learn.adafruit.com/using-canio-circuitpython>`_.
//| """
//|

#include "py/obj.h"
#include "py/enum.h"

#include "shared-bindings/canio/__init__.h"
#include "shared-bindings/canio/CAN.h"
#include "shared-bindings/canio/Match.h"
#include "shared-bindings/canio/Message.h"
#include "shared-bindings/canio/Listener.h"

MAKE_ENUM_VALUE(canio_bus_state_type, bus_state, ERROR_ACTIVE, BUS_STATE_ERROR_ACTIVE);
MAKE_ENUM_VALUE(canio_bus_state_type, bus_state, ERROR_PASSIVE, BUS_STATE_ERROR_PASSIVE);
MAKE_ENUM_VALUE(canio_bus_state_type, bus_state, ERROR_WARNING, BUS_STATE_ERROR_WARNING);
MAKE_ENUM_VALUE(canio_bus_state_type, bus_state, BUS_OFF, BUS_STATE_OFF);

//| class BusState:
//|     """The state of the CAN bus"""
//|
//|     ERROR_ACTIVE: object
//|     """The bus is in the normal (active) state"""
//|
//|     ERROR_WARNING: object
//|     """The bus is in the normal (active) state, but a moderate number of errors have occurred recently.
//|
//|     .. note:: Not all implementations may use ``ERROR_WARNING``.  Do not rely on seeing ``ERROR_WARNING`` before ``ERROR_PASSIVE``."""
//|
//|     ERROR_PASSIVE: object
//|     """The bus is in the passive state due to the number of errors that have occurred recently.
//|
//|     This device will acknowledge packets it receives, but cannot transmit messages.
//|     If additional errors occur, this device may progress to BUS_OFF.
//|     If it successfully acknowledges other packets on the bus, it can return to ERROR_WARNING or ERROR_ACTIVE and transmit packets.
//|     """
//|
//|     BUS_OFF: object
//|     """The bus has turned off due to the number of errors that have
//|     occurred recently.  It must be restarted before it will send or receive
//|     packets.  This device will neither send or acknowledge packets on the bus."""
//|
MAKE_ENUM_MAP(canio_bus_state) {
    MAKE_ENUM_MAP_ENTRY(bus_state, ERROR_ACTIVE),
    MAKE_ENUM_MAP_ENTRY(bus_state, ERROR_PASSIVE),
    MAKE_ENUM_MAP_ENTRY(bus_state, ERROR_WARNING),
    MAKE_ENUM_MAP_ENTRY(bus_state, BUS_OFF),
};
static MP_DEFINE_CONST_DICT(canio_bus_state_locals_dict, canio_bus_state_locals_table);

MAKE_PRINTER(canio, canio_bus_state);

MAKE_ENUM_TYPE(canio, BusState, canio_bus_state);

static const mp_rom_map_elem_t canio_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_canio) },
    { MP_ROM_QSTR(MP_QSTR_BusState), MP_ROM_PTR(&canio_bus_state_type) },
    { MP_ROM_QSTR(MP_QSTR_CAN), MP_ROM_PTR(&canio_can_type) },
    { MP_ROM_QSTR(MP_QSTR_Listener), MP_ROM_PTR(&canio_listener_type) },
    { MP_ROM_QSTR(MP_QSTR_Match), MP_ROM_PTR(&canio_match_type) },
    { MP_ROM_QSTR(MP_QSTR_Message), MP_ROM_PTR(&canio_message_type) },
    { MP_ROM_QSTR(MP_QSTR_RemoteTransmissionRequest), MP_ROM_PTR(&canio_remote_transmission_request_type) },
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR__canio) },
};

static MP_DEFINE_CONST_DICT(canio_module_globals, canio_module_globals_table);

const mp_obj_module_t canio_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&canio_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_canio, canio_module);
