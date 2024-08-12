// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Jeff Epler for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#include "hal/utils/include/utils.h"
#include "component/can.h"

#define COMMON_HAL_CANIO_MAX_MESSAGE_LENGTH (8)
#define COMMON_HAL_CANIO_RX_FIFO_SIZE (3)
#define COMMON_HAL_CANIO_RX_FILTER_SIZE (4)
#define COMMON_HAL_CANIO_TX_FIFO_SIZE (1)

// This appears to be a typo (transposition error) in the ASF4 headers
// It's called the "Extended ID Filter Entry"
typedef CanMramXifde CanMramXidfe;

typedef struct canio_listener canio_listener_t;
typedef struct canio_can canio_can_t;

typedef struct {
    CAN_TXBE_0_Type txb0;
    CAN_TXBE_1_Type txb1;
    COMPILER_ALIGNED(4)
    uint8_t data[COMMON_HAL_CANIO_MAX_MESSAGE_LENGTH];
} canio_can_tx_buffer_t;

typedef struct {
    CAN_RXF0E_0_Type rxf0;
    CAN_RXF0E_1_Type rxf1;
    COMPILER_ALIGNED(4)
    uint8_t data[COMMON_HAL_CANIO_MAX_MESSAGE_LENGTH];
} canio_can_rx_fifo_t;

typedef uint32_t canio_can_filter_t;

typedef struct {
    canio_can_tx_buffer_t tx_buffer[COMMON_HAL_CANIO_TX_FIFO_SIZE];
    canio_can_rx_fifo_t rx0_fifo[COMMON_HAL_CANIO_RX_FIFO_SIZE];
    canio_can_rx_fifo_t rx1_fifo[COMMON_HAL_CANIO_RX_FIFO_SIZE];
    CanMramSidfe standard_rx_filter[COMMON_HAL_CANIO_RX_FILTER_SIZE];
    CanMramXifde extended_rx_filter[COMMON_HAL_CANIO_RX_FILTER_SIZE];
} canio_can_state_t;
