// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2016 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT
#include "hpl_gpio.h"

#include "py/mphal.h"

#include "shared-bindings/neopixel_write/__init__.h"

#include "supervisor/port.h"

#if defined(SAME54)
#include "hri/hri_cmcc_e54.h"
#include "hri/hri_nvmctrl_e54.h"
#elif defined(SAME51)
#include "hri/hri_cmcc_e51.h"
#include "hri/hri_nvmctrl_e51.h"
#elif defined(SAMD51)
#include "hri/hri_cmcc_d51.h"
#include "hri/hri_nvmctrl_d51.h"
#endif

__attribute__((naked, noinline, aligned(16)))
static void neopixel_send_buffer_core(volatile uint32_t *clraddr, uint32_t pinMask,
    const uint8_t *ptr, int numBytes);

// The SAMD21 timing loop durations below are approximate,
// because the other instructions take significant time.

static void neopixel_send_buffer_core(volatile uint32_t *clraddr, uint32_t pinMask,
    const uint8_t *ptr, int numBytes) {
    asm volatile ("        push    {r4, r5, r6, lr};"
        "        add     r3, r2, r3;"
        "loopLoad:"
        "        ldrb r5, [r2, #0];"          // r5 := *ptr
        "        add  r2, #1;"                // ptr++
        "        movs    r4, #128;"           // r4-mask, 0x80

        "loopBit:"
        "        str r1, [r0, #4];"                             // set
        #ifdef SAMD21
        "        movs r6, #2; d2: sub r6, #1; bne d2;"          // 248 ns high (entire T0H or start T1H)
        #endif
        #ifdef SAM_D5X_E5X
        "        movs r6, #11; d2: subs r6, #1; bne d2;"        // 300 ns high (entire T0H or start T1H)
        #endif
        "        tst r4, r5;"                                   // mask&r5
        "        bne skipclr;"
        "        str r1, [r0, #0];"          // clr

        "skipclr:"
        #ifdef SAMD21
        "        movs r6, #7; d0: sub r6, #1; bne d0;"          // 772 ns low or high (start T0L or end T1H)
        #endif
        #ifdef SAM_D5X_E5X
        "        movs r6, #15; d0: subs r6, #1; bne d0;"        // 388 ns low or high (start T0L or end T1H)
        #endif
        "        str r1, [r0, #0];"            // clr (possibly again, doesn't matter)

        #ifdef SAMD21
        "        asr     r4, r4, #1;"          // mask >>= 1
        #endif
        #ifdef SAM_D5X_E5X
        "        asrs     r4, r4, #1;"          // mask >>= 1
        #endif
        "        beq     nextbyte;"
        "        uxtb    r4, r4;"
        #ifdef SAMD21
        "        movs r6, #5; d1: sub r6, #1; bne d1;"          // 496 ns (end TOL or entire T1L)
        #endif
        #ifdef SAM_D5X_E5X
        "        movs r6, #20; d1: subs r6, #1; bne d1;"        // 548 ns (end TOL or entire T1L)
        #endif
        "        b       loopBit;"

        "nextbyte:"
        #ifdef SAMD21
        "        movs r6, #1; d3: sub r6, #1; bne d3;"          // 60 ns (end TOL or entire T1L)
                                                                // other instructions add more delay
        #endif
        #ifdef SAM_D5X_E5X
        "        movs r6, #18; d3: subs r6, #1; bne d3;"        // extra for 936 ns total (byte end T0L or entire T1L)
        #endif
        "        cmp r2, r3;"
        "        bcs neopixel_stop;"
        "        b loopLoad;"
        "neopixel_stop:"
        "        pop {r4, r5, r6, pc};"
        "");
}

static uint64_t next_start_raw_ticks = 0;

void common_hal_neopixel_write(const digitalio_digitalinout_obj_t *digitalinout, uint8_t *pixels, uint32_t numBytes) {
    // This is adapted directly from the Adafruit NeoPixel library SAMD21G18A code:
    // https://github.com/adafruit/Adafruit_NeoPixel/blob/master/Adafruit_NeoPixel.cpp
    // and the asm version from https://github.com/microsoft/uf2-samdx1/blob/master/inc/neopixel.h
    uint32_t pinMask;
    PortGroup *port;

    // Wait to make sure we don't append onto the last transmission. This should only be a tick or
    // two.
    while (port_get_raw_ticks(NULL) < next_start_raw_ticks) {
    }

    // Turn off interrupts of any kind during timing-sensitive code.
    mp_hal_disable_all_interrupts();

    uint32_t pin = digitalinout->pin->number;
    port = &PORT->Group[GPIO_PORT(pin)];      // Convert GPIO # to port register
    pinMask = (1UL << (pin % 32));   // From port_pin_set_output_level ASF code.
    volatile uint32_t *clr = &(port->OUTCLR.reg);
    neopixel_send_buffer_core(clr, pinMask, pixels, numBytes);

    // Update the next start.
    next_start_raw_ticks = port_get_raw_ticks(NULL) + 4;

    // Turn on interrupts after timing-sensitive code.
    mp_hal_enable_all_interrupts();

}
