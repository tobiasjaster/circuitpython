// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/neopixel_write/__init__.h"

#include "bindings/rp2pio/StateMachine.h"
#include "common-hal/rp2pio/StateMachine.h"
#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/digitalio/DigitalInOut.h"

#include "supervisor/port.h"

uint64_t next_start_raw_ticks = 0;

// NeoPixels are 800khz bit streams. We are choosing zeros as <312ns hi, 936 lo> and ones
// and ones as <700 ns hi, 556 ns lo>.
// cycle. The first two instructions always run while only one of the two final
// instructions run per bit. We start with the low period because it can be
// longer while waiting for more data.
const uint16_t neopixel_program[] = {
// bitloop:
//   out x 1        side 0 [6]; Drive low. Side-set still takes place before instruction stalls.
    0x6621,
//   jmp !x do_zero side 1 [3]; Branch on the bit we shifted out previous delay. Drive high.
    0x1323,
// do_one:
//   jmp  bitloop   side 1 [4]; Continue driving high, for a one (long pulse)
    0x1400,
// do_zero:
//   nop            side 0 [4]; Or drive low, for a zero (short pulse)
    0xa442
};

void common_hal_neopixel_write(const digitalio_digitalinout_obj_t *digitalinout, uint8_t *pixels, uint32_t num_bytes) {
    // Set everything up.
    rp2pio_statemachine_obj_t state_machine;

    // TODO: Cache the state machine after we create it once. We'll need a way to
    // change the pins then though.
    uint32_t pins_we_use = 1 << digitalinout->pin->number;
    bool ok = rp2pio_statemachine_construct(&state_machine,
        neopixel_program, MP_ARRAY_SIZE(neopixel_program),
        12800000, // 12.8MHz, to get appropriate sub-bit times in PIO program.
        NULL, 0, // init program
        NULL, 1, // out
        NULL, 1, // in
        0, 0, // in pulls
        NULL, 1, // set
        digitalinout->pin, 1, // sideset
        0, pins_we_use, // initial pin state
        NULL, // jump pin
        pins_we_use, true, false,
        true, 8, false, // TX, auto pull every 8 bits. shift left to output msb first
        true, // Wait for txstall. If we don't, then we'll deinit too quickly.
        false, 32, true, // RX setting we don't use
        false, // claim pins
        false, // Not user-interruptible.
        false, // No sideset enable
        0, -1, // wrap
        PIO_ANY_OFFSET  // offset
        );
    if (!ok) {
        // Do nothing. Maybe bitbang?
        return;
    }

    // Wait to make sure we don't append onto the last transmission. This should only be a tick or
    // two.
    while (port_get_raw_ticks(NULL) < next_start_raw_ticks) {
    }

    common_hal_rp2pio_statemachine_write(&state_machine, pixels, num_bytes, 1 /* stride in bytes */, false);

    // Use a private deinit of the state machine that doesn't reset the pin.
    rp2pio_statemachine_deinit(&state_machine, true);

    // Reset the pin and release it from the PIO
    gpio_init(digitalinout->pin->number);
    common_hal_digitalio_digitalinout_switch_to_output((digitalio_digitalinout_obj_t *)digitalinout, false, DRIVE_MODE_PUSH_PULL);

    // Update the next start to +2 ticks. This ensures we give it at least 300us.
    next_start_raw_ticks = port_get_raw_ticks(NULL) + 2;
}
