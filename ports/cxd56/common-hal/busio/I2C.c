// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright 2019 Sony Semiconductor Solutions Corporation
//
// SPDX-License-Identifier: MIT

#include <arch/chip/pin.h>
#include <nuttx/i2c/i2c_master.h>
#include <cxd56_i2c.h>
#include <cxd56_pinconfig.h>

#include "py/runtime.h"

#include "shared-bindings/busio/I2C.h"
#include "shared-bindings/microcontroller/Pin.h"

void common_hal_busio_i2c_construct(busio_i2c_obj_t *self, const mcu_pin_obj_t *scl,
    const mcu_pin_obj_t *sda, uint32_t frequency, uint32_t timeout) {
    if (frequency != I2C_SPEED_STANDARD && frequency != I2C_SPEED_FAST) {
        mp_arg_error_invalid(MP_QSTR_frequency);
    }

    if (scl->number != PIN_I2C0_BCK || sda->number != PIN_I2C0_BDT) {
        raise_ValueError_invalid_pins();
    }

    claim_pin(scl);
    claim_pin(sda);

    self->scl_pin = scl;
    self->sda_pin = sda;
    self->frequency = frequency;
    self->i2c_dev = cxd56_i2cbus_initialize(0);
    CXD56_PIN_CONFIGS(PINCONFS_I2C0);
}

void common_hal_busio_i2c_deinit(busio_i2c_obj_t *self) {
    if (common_hal_busio_i2c_deinited(self)) {
        return;
    }

    cxd56_i2cbus_uninitialize(self->i2c_dev);
    self->i2c_dev = NULL;

    reset_pin_number(self->scl_pin->number);
    reset_pin_number(self->sda_pin->number);
}

bool common_hal_busio_i2c_deinited(busio_i2c_obj_t *self) {
    return self->i2c_dev == NULL;
}

bool common_hal_busio_i2c_try_lock(busio_i2c_obj_t *self) {
    bool grabbed_lock = false;
    if (!self->has_lock) {
        grabbed_lock = true;
        self->has_lock = true;
    }
    return grabbed_lock;
}

bool common_hal_busio_i2c_has_lock(busio_i2c_obj_t *self) {
    return self->has_lock;
}

void common_hal_busio_i2c_unlock(busio_i2c_obj_t *self) {
    self->has_lock = false;
}

bool common_hal_busio_i2c_probe(busio_i2c_obj_t *self, uint8_t addr) {
    struct i2c_msg_s msg;

    msg.frequency = self->frequency;
    msg.addr = addr;
    msg.flags = 0;
    msg.buffer = NULL;
    msg.length = 0;
    return I2C_TRANSFER(self->i2c_dev, &msg, 1) < 0 ? false : true;
}

static uint8_t _common_hal_busio_i2c_write(busio_i2c_obj_t *self, uint16_t address, const uint8_t *data, size_t len, bool stop) {
    struct i2c_msg_s msg;

    msg.frequency = self->frequency;
    msg.addr = address;
    msg.flags = (stop ? 0 : I2C_M_NOSTOP);
    msg.buffer = (uint8_t *)data;
    msg.length = len;
    return -I2C_TRANSFER(self->i2c_dev, &msg, 1);
}

uint8_t common_hal_busio_i2c_write(busio_i2c_obj_t *self, uint16_t addr,
    const uint8_t *data, size_t len) {
    return _common_hal_busio_i2c_write(self, addr, data, len, true);
}

uint8_t common_hal_busio_i2c_read(busio_i2c_obj_t *self, uint16_t address, uint8_t *data, size_t len) {
    struct i2c_msg_s msg;

    msg.frequency = self->frequency;
    msg.addr = address;
    msg.flags = I2C_M_READ;
    msg.buffer = data;
    msg.length = len;
    return -I2C_TRANSFER(self->i2c_dev, &msg, 1);
}

uint8_t common_hal_busio_i2c_write_read(busio_i2c_obj_t *self, uint16_t addr,
    uint8_t *out_data, size_t out_len, uint8_t *in_data, size_t in_len) {
    uint8_t result = _common_hal_busio_i2c_write(self, addr, out_data, out_len, false);
    if (result != 0) {
        return result;
    }

    return common_hal_busio_i2c_read(self, addr, in_data, in_len);
}

void common_hal_busio_i2c_never_reset(busio_i2c_obj_t *self) {
    never_reset_pin_number(self->scl_pin->number);
    never_reset_pin_number(self->sda_pin->number);
}
