// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright 2019 Sony Semiconductor Solutions Corporation
//
// SPDX-License-Identifier: MIT

#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <nuttx/serial/tioctl.h>
#include <nuttx/fs/ioctl.h>

#include "py/mperrno.h"
#include "py/stream.h"
#include "py/runtime.h"

#include "shared-bindings/busio/UART.h"
#include "shared-bindings/microcontroller/Pin.h"

typedef struct {
    const char *devpath;
    const mcu_pin_obj_t *tx;
    const mcu_pin_obj_t *rx;
    int fd;
} busio_uart_dev_t;

static busio_uart_dev_t busio_uart_dev[] = {
    {"/dev/ttyS2", &pin_UART2_TXD, &pin_UART2_RXD, -1},
};

void common_hal_busio_uart_construct(busio_uart_obj_t *self,
    const mcu_pin_obj_t *tx, const mcu_pin_obj_t *rx,
    const mcu_pin_obj_t *rts, const mcu_pin_obj_t *cts,
    const mcu_pin_obj_t *rs485_dir, bool rs485_invert,
    uint32_t baudrate, uint8_t bits, busio_uart_parity_t parity, uint8_t stop,
    mp_float_t timeout, uint16_t receiver_buffer_size, byte *receiver_buffer,
    bool sigint_enabled) {
    int i;
    int count;
    char tmp;
    struct termios tio;

    if ((rts != NULL) || (cts != NULL) || (rs485_dir != NULL) || (rs485_invert)) {
        mp_raise_NotImplementedError(MP_ERROR_TEXT("RS485"));
    }

    mp_arg_validate_int(bits, 8, MP_QSTR_bits);
    mp_arg_validate_int(parity, BUSIO_UART_PARITY_NONE, MP_QSTR_parity);
    mp_arg_validate_int(stop, 1, MP_QSTR_stop);

    self->number = -1;

    for (int i = 0; i < MP_ARRAY_SIZE(busio_uart_dev); i++) {
        if (tx->number == busio_uart_dev[i].tx->number &&
            rx->number == busio_uart_dev[i].rx->number) {
            self->number = i;
            break;
        }
    }

    if (self->number < 0) {
        raise_ValueError_invalid_pins();
    }

    if (busio_uart_dev[self->number].fd < 0) {
        busio_uart_dev[self->number].fd = open(busio_uart_dev[self->number].devpath, O_RDWR);
        if (busio_uart_dev[self->number].fd < 0) {
            mp_raise_RuntimeError(MP_ERROR_TEXT("UART init"));
        }

        // Wait to make sure the UART is ready
        usleep(1000);
        // Clear RX FIFO
        ioctl(busio_uart_dev[self->number].fd, FIONREAD, (long unsigned int)&count);
        for (i = 0; i < count; i++) {
            read(busio_uart_dev[self->number].fd, &tmp, 1);
        }
    }

    ioctl(busio_uart_dev[self->number].fd, TCGETS, (long unsigned int)&tio);
    tio.c_speed = baudrate;
    ioctl(busio_uart_dev[self->number].fd, TCSETS, (long unsigned int)&tio);
    ioctl(busio_uart_dev[self->number].fd, TCFLSH, (long unsigned int)NULL);

    claim_pin(tx);
    claim_pin(rx);

    self->tx_pin = tx;
    self->rx_pin = rx;
    self->baudrate = baudrate;
    self->timeout_us = timeout * 1000000;
}

void common_hal_busio_uart_deinit(busio_uart_obj_t *self) {
    if (common_hal_busio_uart_deinited(self)) {
        return;
    }

    close(busio_uart_dev[self->number].fd);
    busio_uart_dev[self->number].fd = -1;

    reset_pin_number(self->tx_pin->number);
    reset_pin_number(self->rx_pin->number);
}

bool common_hal_busio_uart_deinited(busio_uart_obj_t *self) {
    return busio_uart_dev[self->number].fd < 0;
}

size_t common_hal_busio_uart_read(busio_uart_obj_t *self, uint8_t *data, size_t len, int *errcode) {
    fd_set rfds;
    struct timeval tv;
    int retval, bytes_read;

    // make sure we want at least 1 char
    if (len == 0) {
        return 0;
    }

    FD_ZERO(&rfds);
    FD_SET(busio_uart_dev[self->number].fd, &rfds);

    tv.tv_sec = 0;
    tv.tv_usec = self->timeout_us;

    retval = select(busio_uart_dev[self->number].fd + 1, &rfds, NULL, NULL, &tv);

    if (retval) {
        bytes_read = read(busio_uart_dev[self->number].fd, data, len);
    } else {
        *errcode = EAGAIN;
        return MP_STREAM_ERROR;
    }

    return bytes_read;
}

size_t common_hal_busio_uart_write(busio_uart_obj_t *self, const uint8_t *data, size_t len, int *errcode) {
    int bytes_written = write(busio_uart_dev[self->number].fd, data, len);
    if (bytes_written < 0) {
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }

    return bytes_written;
}

uint32_t common_hal_busio_uart_get_baudrate(busio_uart_obj_t *self) {
    return self->baudrate;
}

void common_hal_busio_uart_set_baudrate(busio_uart_obj_t *self, uint32_t baudrate) {
    struct termios tio;

    ioctl(busio_uart_dev[self->number].fd, TCGETS, (long unsigned int)&tio);
    tio.c_speed = baudrate;
    ioctl(busio_uart_dev[self->number].fd, TCSETS, (long unsigned int)&tio);
    ioctl(busio_uart_dev[self->number].fd, TCFLSH, (long unsigned int)NULL);
}

mp_float_t common_hal_busio_uart_get_timeout(busio_uart_obj_t *self) {
    return (mp_float_t)(self->timeout_us / 1000000.0f);
}

void common_hal_busio_uart_set_timeout(busio_uart_obj_t *self, mp_float_t timeout) {
    self->timeout_us = timeout * 1000000;
}

uint32_t common_hal_busio_uart_rx_characters_available(busio_uart_obj_t *self) {
    int count = 0;

    ioctl(busio_uart_dev[self->number].fd, FIONREAD, (long unsigned int)&count);

    return count;
}

void common_hal_busio_uart_clear_rx_buffer(busio_uart_obj_t *self) {
}

bool common_hal_busio_uart_ready_to_tx(busio_uart_obj_t *self) {
    ioctl(busio_uart_dev[self->number].fd, TCFLSH, (long unsigned int)NULL);
    return true;
}

void busio_uart_reset(void) {
    for (int i = 0; i < MP_ARRAY_SIZE(busio_uart_dev); i++) {
        if (busio_uart_dev[i].fd >= 0) {
            close(busio_uart_dev[i].fd);
            busio_uart_dev[i].fd = -1;
        }
    }
}
