// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 Jeff Epler for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "py/obj.h"
#include "py/runtime.h"

#include "shared/runtime/context_manager_helpers.h"
#include "shared/runtime/interrupt_char.h"

#include "bindings/rp2pio/StateMachine.h"
#include "bindings/rp2pio/__init__.h"
#include "common-hal/imagecapture/ParallelImageCapture.h"
#include "shared-bindings/imagecapture/ParallelImageCapture.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/microcontroller/Processor.h"
#include "shared-bindings/microcontroller/__init__.h"

#include "src/rp2_common/hardware_pio/include/hardware/pio.h"
#include "src/rp2_common/hardware_pio/include/hardware/pio_instructions.h"

// Define this to (1), and you can scope the instruction-pointer of the state machine on D26..28 (note the weird encoding though!)
#define DEBUG_STATE_MACHINE (0)
#if DEBUG_STATE_MACHINE
#define SIDE(x) ((x) << 8)
#else
#define SIDE(x) (0)
#endif

#define _0 SIDE(0b11100)
#define _1 SIDE(0b00000)
#define _2 SIDE(0b10000)
#define _3 SIDE(0b10100)
#define _4 SIDE(0b11000)
#define _5 SIDE(0b10100)

#define IMAGECAPTURE_CODE(width, pclk, vsync, href) \
    { \
/* 0 */ pio_encode_wait_gpio(0, vsync) | _0, \
/* 1 */ pio_encode_wait_gpio(1, vsync) | _1, \
        /* .wrap_target */  \
/* 2 */ pio_encode_wait_gpio(1, href) | _2, \
/* 3 */ pio_encode_wait_gpio(1, pclk) | _3, \
/* 4 */ pio_encode_in(pio_pins, width) | _4, \
/* 5 */ pio_encode_wait_gpio(0, pclk) | _5, \
        /* .wrap */ \
    }

static mcu_pin_obj_t *pin_from_number(uint8_t number) {
    const mp_map_t *mcu_map = &mcu_pin_globals.map;
    for (uint8_t i = 0; i < mcu_map->alloc; i++) {
        mp_obj_t val = mcu_map->table[i].value;
        if (!mp_obj_is_type(val, &mcu_pin_type)) {
            continue;
        }
        mcu_pin_obj_t *pin = MP_OBJ_TO_PTR(val);
        if (pin->number == number) {
            return pin;
        }
    }
    return NULL;
}

void common_hal_imagecapture_parallelimagecapture_construct(imagecapture_parallelimagecapture_obj_t *self,
    const uint8_t data_pins[],
    uint8_t data_count,
    const mcu_pin_obj_t *data_clock,
    const mcu_pin_obj_t *vertical_sync,
    const mcu_pin_obj_t *horizontal_reference) {

    for (int i = 1; i < data_count; i++) {
        if (data_pins[i] - data_pins[0] != i) {
            mp_raise_RuntimeError(MP_ERROR_TEXT("Pins must be sequential"));
        }
    }

    uint16_t imagecapture_code[] = IMAGECAPTURE_CODE(data_count, data_clock->number, vertical_sync->number, horizontal_reference->number);

    common_hal_rp2pio_statemachine_construct(&self->state_machine,
        imagecapture_code, MP_ARRAY_SIZE(imagecapture_code),
        common_hal_mcu_processor_get_frequency(), // full speed (4 instructions per loop -> max pclk 30MHz @ 120MHz)
        0, 0, // init
        NULL, 0, // may_exec
        NULL, 0, 0, 0, // out pins
        pin_from_number(data_pins[0]), data_count, // in pins
        0, 0, // in pulls
        NULL, 0, 0, 0, // set pins
        #if DEBUG_STATE_MACHINE
        &pin_GPIO26, 3, 7, 7, // sideset pins
        #else
        NULL, 0, 0, 0, // sideset pins
        #endif
        false, // No sideset enable
        NULL, PULL_NONE, // jump pin
        (1 << vertical_sync->number) | (1 << horizontal_reference->number) | (1 << data_clock->number), // wait gpio pins
        true, // exclusive pin use
        false, 32, false, // out settings
        false, // wait for txstall
        true, 32, true,  // in settings
        false, // Not user-interruptible.
        2, 5, // wrap settings
        PIO_ANY_OFFSET);
}

void common_hal_imagecapture_parallelimagecapture_deinit(imagecapture_parallelimagecapture_obj_t *self) {
    if (common_hal_imagecapture_parallelimagecapture_deinited(self)) {
        return;
    }
    return common_hal_rp2pio_statemachine_deinit(&self->state_machine);
}

bool common_hal_imagecapture_parallelimagecapture_deinited(imagecapture_parallelimagecapture_obj_t *self) {
    return common_hal_rp2pio_statemachine_deinited(&self->state_machine);
}

void common_hal_imagecapture_parallelimagecapture_singleshot_capture(imagecapture_parallelimagecapture_obj_t *self, mp_obj_t buffer) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buffer, &bufinfo, MP_BUFFER_RW);

    PIO pio = self->state_machine.pio;
    uint sm = self->state_machine.state_machine;
    uint8_t offset = rp2pio_statemachine_program_offset(&self->state_machine);

    pio_sm_set_enabled(pio, sm, false);
    pio_sm_clear_fifos(pio, sm);

    pio_sm_restart(pio, sm);
    pio_sm_exec(pio, sm, pio_encode_jmp(offset));
    pio_sm_set_enabled(pio, sm, true);

    common_hal_rp2pio_statemachine_readinto(&self->state_machine, bufinfo.buf, bufinfo.len, 4, false);

    pio_sm_set_enabled(pio, sm, false);
}
