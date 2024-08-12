// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright 2019 Sony Semiconductor Solutions Corporation
//
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include <sys/boardctl.h>
#include <sys/time.h>

#include <cxd56_rtc.h>

#include "sched/sched.h"

#include "shared-bindings/rtc/__init__.h"

#include "supervisor/board.h"
#include "supervisor/port.h"
#include "supervisor/background_callback.h"
#include "supervisor/usb.h"
#include "supervisor/shared/tick.h"

#include "common-hal/microcontroller/Pin.h"
#include "common-hal/analogio/AnalogIn.h"
#include "common-hal/busio/UART.h"

#define SPRESENSE_MEM_ALIGN (32)

uint32_t *heap;
uint32_t heap_size;

safe_mode_t port_init(void) {
    boardctl(BOARDIOC_INIT, 0);

    // Wait until RTC is available
    while (g_rtc_enabled == false) {
        ;
    }

    heap = memalign(SPRESENSE_MEM_ALIGN, 128 * 1024);
    uint32_t size = CONFIG_RAM_START + CONFIG_RAM_SIZE - (uint32_t)heap - 2 * SPRESENSE_MEM_ALIGN;
    heap = realloc(heap, size);
    heap_size = size / sizeof(uint32_t);

    if (board_requests_safe_mode()) {
        return SAFE_MODE_USER;
    }

    return SAFE_MODE_NONE;
}

void reset_cpu(void) {
    boardctl(BOARDIOC_RESET, 0);
    for (;;) {
    }
}

void reset_port(void) {
    #if CIRCUITPY_ANALOGIO
    analogin_reset();
    #endif
    #if CIRCUITPY_BUSIO
    busio_uart_reset();
    #endif
    #if CIRCUITPY_RTC
    rtc_reset();
    #endif

    reset_all_pins();
}

void reset_to_bootloader(void) {
    boardctl(BOARDIOC_RESET, 0);
    for (;;) {
    }
}

uint32_t *port_stack_get_limit(void) {
    struct tcb_s *rtcb = this_task();

    return rtcb->stack_base_ptr;
}

uint32_t *port_stack_get_top(void) {
    struct tcb_s *rtcb = this_task();

    return rtcb->stack_base_ptr + (uint32_t)rtcb->adj_stack_size;
}

uint32_t *port_heap_get_bottom(void) {
    return heap;
}

uint32_t *port_heap_get_top(void) {
    return heap + heap_size;
}

extern uint32_t _ebss;

// Place the word to save just after our BSS section that gets blanked.
void port_set_saved_word(uint32_t value) {
    _ebss = value;
}

uint32_t port_get_saved_word(void) {
    return _ebss;
}

static background_callback_t callback;
static void usb_background_do(void *unused) {
    usb_background();
}

volatile bool _tick_enabled;
void board_timerhook(void) {
    // Do things common to all ports when the tick occurs
    if (_tick_enabled) {
        supervisor_tick();
    }

    background_callback_add(&callback, usb_background_do, NULL);
}

uint64_t port_get_raw_ticks(uint8_t *subticks) {
    uint64_t count = cxd56_rtc_count();
    *subticks = count % 32;

    return count / 32;
}

// Enable 1/1024 second tick.
void port_enable_tick(void) {
    _tick_enabled = true;
}

// Disable 1/1024 second tick.
void port_disable_tick(void) {
    _tick_enabled = false;
}

void port_interrupt_after_ticks(uint32_t ticks) {
}

void port_idle_until_interrupt(void) {
    // TODO: Implement sleep.
}
