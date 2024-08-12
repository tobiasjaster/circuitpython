// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "common-hal/microcontroller/Pin.h"
#include "shared-bindings/onewireio/OneWire.h"
#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/digitalio/DigitalInOut.h"

// Durations are taken from here: https://www.maximintegrated.com/en/app-notes/index.mvp/id/126

void common_hal_onewireio_onewire_construct(onewireio_onewire_obj_t *self,
    const mcu_pin_obj_t *pin) {
    self->pin.base.type = &digitalio_digitalinout_type;
    common_hal_digitalio_digitalinout_construct(&self->pin, pin);
}

bool common_hal_onewireio_onewire_deinited(onewireio_onewire_obj_t *self) {
    return common_hal_digitalio_digitalinout_deinited(&self->pin);
}

void common_hal_onewireio_onewire_deinit(onewireio_onewire_obj_t *self) {
    if (common_hal_onewireio_onewire_deinited(self)) {
        return;
    }
    common_hal_digitalio_digitalinout_deinit(&self->pin);
}

// We use common_hal_mcu_delay_us(). It should not be dependent on interrupts
// to do accurate timekeeping, since we disable interrupts during the delays below.

bool common_hal_onewireio_onewire_reset(onewireio_onewire_obj_t *self) {
    common_hal_mcu_disable_interrupts();
    common_hal_digitalio_digitalinout_switch_to_output(&self->pin, false, DRIVE_MODE_OPEN_DRAIN);
    common_hal_mcu_delay_us(480);
    common_hal_digitalio_digitalinout_switch_to_input(&self->pin, PULL_NONE);
    common_hal_mcu_delay_us(70);
    bool value = common_hal_digitalio_digitalinout_get_value(&self->pin);
    common_hal_mcu_delay_us(410);
    // test if bus returned high (idle) and not stuck at low
    bool idle = common_hal_digitalio_digitalinout_get_value(&self->pin);
    common_hal_mcu_enable_interrupts();
    return value || !idle;
}

bool common_hal_onewireio_onewire_read_bit(onewireio_onewire_obj_t *self) {
    common_hal_mcu_disable_interrupts();
    common_hal_digitalio_digitalinout_switch_to_output(&self->pin, false, DRIVE_MODE_OPEN_DRAIN);
    common_hal_mcu_delay_us(6);
    common_hal_digitalio_digitalinout_switch_to_input(&self->pin, PULL_NONE);
    // TODO(tannewt): Test with more devices and maybe make the delays
    // configurable. This should be 9 by the datasheet but all bits read as 1
    // then.
    common_hal_mcu_delay_us(6);
    bool value = common_hal_digitalio_digitalinout_get_value(&self->pin);
    common_hal_mcu_delay_us(55);
    common_hal_mcu_enable_interrupts();
    return value;
}

void common_hal_onewireio_onewire_write_bit(onewireio_onewire_obj_t *self,
    bool bit) {
    common_hal_mcu_disable_interrupts();
    common_hal_digitalio_digitalinout_switch_to_output(&self->pin, false, DRIVE_MODE_OPEN_DRAIN);
    common_hal_mcu_delay_us(bit? 6 : 60);
    common_hal_digitalio_digitalinout_switch_to_input(&self->pin, PULL_NONE);
    common_hal_mcu_delay_us(bit? 64 : 10);
    common_hal_mcu_enable_interrupts();
}
