// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2021 Junji Sakai
//
// SPDX-License-Identifier: MIT

#include "supervisor/port.h"

#include <stdint.h>
#include "supervisor/background_callback.h"
#include "supervisor/board.h"

#include "nrfx/hal/nrf_clock.h"
#include "nrfx/hal/nrf_power.h"
#include "nrfx/drivers/include/nrfx_gpiote.h"
#include "nrfx/drivers/include/nrfx_power.h"
#include "nrfx/drivers/include/nrfx_rtc.h"

#include "nrf/cache.h"
#include "nrf/clocks.h"
#include "nrf/power.h"
#include "nrf/timers.h"

#include "nrf_nvic.h"

#include "common-hal/microcontroller/Pin.h"
#include "common-hal/alarm/time/TimeAlarm.h"
#include "common-hal/analogio/AnalogIn.h"
#include "common-hal/busio/I2C.h"
#include "common-hal/busio/SPI.h"
#include "common-hal/busio/UART.h"
#include "common-hal/rtc/RTC.h"
#include "common-hal/neopixel_write/__init__.h"
#include "common-hal/watchdog/WatchDogTimer.h"
#include "common-hal/alarm/__init__.h"

#include "shared-bindings/_bleio/__init__.h"
#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/rtc/__init__.h"

#include "lib/tinyusb/src/device/usbd.h"

#if CIRCUITPY_AUDIOBUSIO
#include "common-hal/audiobusio/I2SOut.h"
#endif

#if CIRCUITPY_AUDIOPWMIO
#include "common-hal/audiopwmio/PWMAudioOut.h"
#endif

#if defined(MICROPY_QSPI_CS)
extern void qspi_disable(void);
#endif

static void power_warning_handler(void) {
    reset_into_safe_mode(SAFE_MODE_BROWNOUT);
}

uint32_t reset_reason_saved = 0;
const nrfx_rtc_t rtc_instance = NRFX_RTC_INSTANCE(2);

nrfx_rtc_config_t rtc_config = {
    .prescaler = RTC_FREQ_TO_PRESCALER(0x8000),
    .reliable = 0,
    .tick_latency = 0,
    .interrupt_priority = 6
};

#define OVERFLOW_CHECK_PREFIX 0x2cad564f
#define OVERFLOW_CHECK_SUFFIX 0x11343ef7
static volatile struct {
    uint32_t prefix;
    uint64_t overflowed_ticks;
    uint32_t suffix;
} overflow_tracker __attribute__((section(".uninitialized")));

static void rtc_handler(nrfx_rtc_int_type_t int_type) {
    if (int_type == NRFX_RTC_INT_OVERFLOW) {
        // Our RTC is 24 bits and we're clocking it at 32.768khz which is 32 (2 ** 5) subticks per
        // tick.
        overflow_tracker.overflowed_ticks += (1L << (24 - 5));
    } else if (int_type == NRFX_RTC_INT_TICK && nrfx_rtc_counter_get(&rtc_instance) % 32 == 0) {
        // Do things common to all ports when the tick occurs
        supervisor_tick();
    } else if (int_type == NRFX_RTC_INT_COMPARE0) {
        nrfx_rtc_cc_set(&rtc_instance, 0, 0, false);
    } else if (int_type == NRFX_RTC_INT_COMPARE1) {
        // used in light sleep
        #if CIRCUITPY_ALARM
        sleepmem_wakeup_event = SLEEPMEM_WAKEUP_BY_TIMER;
        #endif
        nrfx_rtc_cc_set(&rtc_instance, 1, 0, false);
    }
}

static void tick_init(void) {
    if (!nrf_clock_lf_is_running(NRF_CLOCK)) {
        nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_LFCLKSTART);
    }
    nrfx_rtc_counter_clear(&rtc_instance);
    nrfx_rtc_init(&rtc_instance, &rtc_config, rtc_handler);
    nrfx_rtc_enable(&rtc_instance);
    nrfx_rtc_overflow_enable(&rtc_instance, true);

    // If the check prefix and suffix aren't correct, then the structure
    // in memory isn't correct and the clock will be wildly wrong. Initialize
    // the prefix and suffix so that we know the value is correct, and reset
    // the time to 0.
    if (overflow_tracker.prefix != OVERFLOW_CHECK_PREFIX ||
        overflow_tracker.suffix != OVERFLOW_CHECK_SUFFIX) {
        overflow_tracker.prefix = OVERFLOW_CHECK_PREFIX;
        overflow_tracker.suffix = OVERFLOW_CHECK_SUFFIX;
        overflow_tracker.overflowed_ticks = 0;
    }
}

static void tick_uninit(void) {
    nrfx_rtc_counter_clear(&rtc_instance);
    nrfx_rtc_disable(&rtc_instance);
    nrfx_rtc_uninit(&rtc_instance);
}

void tick_set_prescaler(uint32_t prescaler_val) {
    tick_uninit();
    // update of prescaler value sometimes fails if we skip this delay..
    NRFX_DELAY_US(1000);
    uint16_t prescaler_saved = rtc_config.prescaler;
    rtc_config.prescaler = prescaler_val;
    tick_init();
    rtc_config.prescaler = prescaler_saved;
}

safe_mode_t port_init(void) {
    nrf_peripherals_clocks_init();

    // If GPIO voltage is set wrong in UICR, this will fix it, and
    // will also do a reset to make the change take effect.
    nrf_peripherals_power_init();

    nrfx_power_pofwarn_config_t power_failure_config;
    power_failure_config.handler = power_warning_handler;
    power_failure_config.thr = NRF_POWER_POFTHR_V27;
    #if NRF_POWER_HAS_VDDH
    power_failure_config.thrvddh = NRF_POWER_POFTHRVDDH_V27;
    #endif
    nrfx_power_pof_init(&power_failure_config);
    nrfx_power_pof_enable(&power_failure_config);

    nrf_peripherals_enable_cache();

    // Configure millisecond timer initialization.
    tick_init();

    #if CIRCUITPY_RTC
    common_hal_rtc_init();
    #endif

    #if CIRCUITPY_ANALOGIO
    analogin_init();
    #endif

    reset_reason_saved = NRF_POWER->RESETREAS;
    // clear all RESET reason bits
    NRF_POWER->RESETREAS = reset_reason_saved;
    // clear wakeup event/pin when reset by reset-pin
    if (reset_reason_saved & NRF_POWER_RESETREAS_RESETPIN_MASK) {
        #if CIRCUITPY_ALARM
        sleepmem_wakeup_event = SLEEPMEM_WAKEUP_BY_NONE;
        #endif
    }

    // If the board was reset by the WatchDogTimer, we may
    // need to boot into safe mode. Reset the RESETREAS bit
    // for the WatchDogTimer so we don't encounter this the
    // next time we reboot.
    if (reset_reason_saved & POWER_RESETREAS_DOG_Msk) {
        NRF_POWER->RESETREAS = POWER_RESETREAS_DOG_Msk;
        uint32_t usb_reg = NRF_POWER->USBREGSTATUS;

        // If USB is connected, then the user might be editing `code.py`,
        // in which case we should reboot into Safe Mode.
        if (usb_reg & POWER_USBREGSTATUS_VBUSDETECT_Msk) {
            return SAFE_MODE_WATCHDOG;
        }
    }

    return SAFE_MODE_NONE;
}

void reset_port(void) {
    #if CIRCUITPY_BUSIO
    i2c_reset();
    spi_reset();
    uart_reset();
    #endif

    #if CIRCUITPY_NEOPIXEL_WRITE
    neopixel_write_reset();
    #endif

    #if CIRCUITPY_AUDIOBUSIO
    i2s_reset();
    #endif

    #if CIRCUITPY_RTC
    rtc_reset();
    #endif

    timers_reset();

    #if CIRCUITPY_WATCHDOG
    watchdog_reset();
    #endif

    // Always reset GPIOTE because it is shared.
    if (nrfx_gpiote_is_init()) {
        nrfx_gpiote_uninit();
    }
    nrfx_gpiote_init(NRFX_GPIOTE_CONFIG_IRQ_PRIORITY);

    reset_all_pins();
}

void reset_to_bootloader(void) {
    enum { DFU_MAGIC_SERIAL = 0x4e };

    NRF_POWER->GPREGRET = DFU_MAGIC_SERIAL;
    reset_cpu();
}

void reset_cpu(void) {
    // We're getting ready to reset, so save the counter off.
    // This counter will get reset to zero during the reboot.
    uint32_t ticks = nrfx_rtc_counter_get(&rtc_instance);
    overflow_tracker.overflowed_ticks += ticks / 32;
    NVIC_SystemReset();
    for (;;) {
    }
}

// The uninitialized data section is placed directly after BSS, under the theory
// that CircuitPython has a lot more .data and .bss than the bootloader.  As a
// result, this section is less likely to be tampered with by the bootloader.
extern uint32_t _euninitialized;

uint32_t *port_heap_get_bottom(void) {
    return &_euninitialized;
}

uint32_t *port_heap_get_top(void) {
    return port_stack_get_limit();
}

uint32_t *port_stack_get_limit(void) {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Warray-bounds"
    return port_stack_get_top() - (CIRCUITPY_DEFAULT_STACK_SIZE + CIRCUITPY_EXCEPTION_STACK_SIZE) / sizeof(uint32_t);
    #pragma GCC diagnostic pop
}

uint32_t *port_stack_get_top(void) {
    return &_estack;
}

// Place the word in the first 32k of RAM. This is saved by us and the
// bootloader for the soft device. We only use it before the soft device uses
// that memory.
#define SAVED_WORD ((uint32_t *)(0x20008000 - 4))
void port_set_saved_word(uint32_t value) {
    *SAVED_WORD = value;
}

uint32_t port_get_saved_word(void) {
    return *SAVED_WORD;
}

uint64_t port_get_raw_ticks(uint8_t *subticks) {
    common_hal_mcu_disable_interrupts();
    uint32_t rtc = nrfx_rtc_counter_get(&rtc_instance);
    uint64_t overflow_count = overflow_tracker.overflowed_ticks;
    common_hal_mcu_enable_interrupts();

    if (subticks != NULL) {
        *subticks = (rtc % 32);
    }
    return overflow_count + rtc / 32;
}

// Enable 1/1024 second tick.
void port_enable_tick(void) {
    nrfx_rtc_tick_enable(&rtc_instance, true);
}

// Disable 1/1024 second tick.
void port_disable_tick(void) {
    nrfx_rtc_tick_disable(&rtc_instance);
}

void port_interrupt_after_ticks_ch(uint32_t channel, uint32_t ticks) {
    uint32_t current_ticks = nrfx_rtc_counter_get(&rtc_instance);
    uint32_t diff = 3;
    if (ticks > diff) {
        diff = ticks * 32;
    }
    if (diff > 0xffffff) {
        diff = 0xffffff;
    }
    nrfx_rtc_cc_set(&rtc_instance, channel, current_ticks + diff, true);
}

void port_disable_interrupt_after_ticks_ch(uint32_t channel) {
    nrfx_rtc_cc_disable(&rtc_instance, channel);
}

void port_interrupt_after_ticks(uint32_t ticks) {
    port_interrupt_after_ticks_ch(0, ticks);
}

void port_idle_until_interrupt(void) {
    #if defined(MICROPY_QSPI_CS)
    qspi_disable();
    #endif

    // Clear the FPU interrupt because it can prevent us from sleeping.
    if (NVIC_GetPendingIRQ(FPU_IRQn)) {
        __set_FPSCR(__get_FPSCR() & ~(0x9f));
        (void)__get_FPSCR();
        NVIC_ClearPendingIRQ(FPU_IRQn);
    }
    uint8_t sd_enabled;

    sd_softdevice_is_enabled(&sd_enabled);
    if (sd_enabled) {
        if (!background_callback_pending()) {
            sd_app_evt_wait();
        }
    } else {
        // Call wait for interrupt ourselves if the SD isn't enabled.
        // Note that `wfi` should be called with interrupts disabled,
        // to ensure that the queue is properly drained.  The `wfi`
        // instruction will returned as long as an interrupt is
        // available, even though the actual handler won't fire until
        // we re-enable interrupts.
        //
        // We do not use common_hal_mcu_disable_interrupts here because
        // we truly require that interrupts be disabled, while
        // common_hal_mcu_disable_interrupts actually just masks the
        // interrupts that are not required to allow the softdevice to
        // function (whether or not SD is enabled)
        int nested = __get_PRIMASK();
        __disable_irq();
        if (!background_callback_pending()) {
            __DSB();
            __WFI();
        }
        if (!nested) {
            __enable_irq();
        }
    }
}


extern void HardFault_Handler(void);
void HardFault_Handler(void) {
    reset_into_safe_mode(SAFE_MODE_HARD_FAULT);
    while (true) {
        asm ("nop;");
    }
}
