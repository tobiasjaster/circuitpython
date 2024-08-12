// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <string.h>
#include <stdlib.h>

#include "supervisor/board.h"
#include "supervisor/port.h"

#include "supervisor/samd_prevent_sleep.h"

// ASF 4
#include "atmel_start_pins.h"
#include "peripheral_clk_config.h"
#include "hal/include/hal_delay.h"
#include "hal/include/hal_flash.h"
#include "hal/include/hal_gpio.h"
#include "hal/include/hal_init.h"
#include "hpl/gclk/hpl_gclk_base.h"
#include "hpl/pm/hpl_pm_base.h"

#if defined(SAMD21)
#include "hri/hri_pm_d21.h"
#elif defined(SAME54)
#include "hri/hri_rstc_e54.h"
#elif defined(SAME51)
#include "sam.h"
#include "hri/hri_rstc_e51.h"
#elif defined(SAMD51)
#include "hri/hri_rstc_d51.h"
#else
#error Unknown chip family
#endif

#if CIRCUITPY_ANALOGIO
#include "common-hal/analogio/AnalogIn.h"
#include "common-hal/analogio/AnalogOut.h"
#endif

#if CIRCUITPY_AUDIOBUSIO
#include "common-hal/audiobusio/PDMIn.h"
#include "common-hal/audiobusio/I2SOut.h"
#endif

#if CIRCUITPY_AUDIOIO
#include "common-hal/audioio/AudioOut.h"
#endif

#if CIRCUITPY_BUSIO
#include "common-hal/busio/__init__.h"
#endif

#if CIRCUITPY_FREQUENCYIO
#include "common-hal/frequencyio/FrequencyIn.h"
#endif

#include "common-hal/microcontroller/Pin.h"

#if CIRCUITPY_PS2IO
#include "common-hal/ps2io/Ps2.h"
#endif

#if CIRCUITPY_RTC
#include "common-hal/rtc/RTC.h"
#endif

#if CIRCUITPY_ALARM
#include "common-hal/alarm/__init__.h"
#include "common-hal/alarm/time/TimeAlarm.h"
#include "common-hal/alarm/pin/PinAlarm.h"
#endif

#if CIRCUITPY_TOUCHIO_USE_NATIVE
#include "common-hal/touchio/TouchIn.h"
#endif

#include "samd/cache.h"
#include "samd/clocks.h"
#include "samd/events.h"
#include "samd/external_interrupts.h"
#include "samd/dma.h"
#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/rtc/__init__.h"
#include "reset.h"

#include "supervisor/background_callback.h"
#include "supervisor/shared/safe_mode.h"
#include "supervisor/shared/stack.h"
#include "supervisor/shared/tick.h"

#include "tusb.h"

#if CIRCUITPY_PEW
#include "common-hal/_pew/PewPew.h"
#endif
static volatile size_t sleep_disable_count = 0;

#ifdef SAMD21
static uint8_t _tick_event_channel = EVSYS_SYNCH_NUM;

static bool tick_enabled(void) {
    return _tick_event_channel != EVSYS_SYNCH_NUM;
}

// Sleeping requires a register write that can stall interrupt handling. Turning
// off sleeps allows for more accurate interrupt timing. (Python still thinks
// it is sleeping though.)
void samd_prevent_sleep(void) {
    sleep_disable_count++;
}

void samd_allow_sleep(void) {
    if (sleep_disable_count == 0) {
        // We should never reach this!
        return;
    }
    sleep_disable_count--;
}
#endif // SAMD21

static void reset_ticks(void) {
    #ifdef SAMD21
    _tick_event_channel = EVSYS_SYNCH_NUM;
    #endif
}

extern volatile bool mp_msc_enabled;

#if defined(SAMD21) && defined(ENABLE_MICRO_TRACE_BUFFER)
// Stores 2 ^ TRACE_BUFFER_MAGNITUDE_PACKETS packets.
// 7 -> 128 packets
#define TRACE_BUFFER_MAGNITUDE_PACKETS 7
// Size in uint32_t. Two per packet.
#define TRACE_BUFFER_SIZE (1 << (TRACE_BUFFER_MAGNITUDE_PACKETS + 1))
// Size in bytes. 4 bytes per uint32_t.
#define TRACE_BUFFER_SIZE_BYTES (TRACE_BUFFER_SIZE << 2)
__attribute__((__aligned__(TRACE_BUFFER_SIZE_BYTES))) uint32_t mtb[TRACE_BUFFER_SIZE] = {0};
#endif

#if CALIBRATE_CRYSTALLESS
static void save_usb_clock_calibration(void) {
    // If we are on USB lets double check our fine calibration for the clock and
    // save the new value if its different enough.
    SYSCTRL->DFLLSYNC.bit.READREQ = 1;
    uint16_t saved_calibration = 0x1ff;
    if (strcmp((char *)CIRCUITPY_INTERNAL_CONFIG_START_ADDR, "CIRCUITPYTHON1") == 0) {
        saved_calibration = ((uint16_t *)CIRCUITPY_INTERNAL_CONFIG_START_ADDR)[8];
    }
    while (SYSCTRL->PCLKSR.bit.DFLLRDY == 0) {
        // TODO(tannewt): Run the mass storage stuff if this takes a while.
    }
    int16_t current_calibration = SYSCTRL->DFLLVAL.bit.FINE;
    if (abs(current_calibration - saved_calibration) > 10) {
        // Copy the full internal config page to memory.
        uint8_t page_buffer[NVMCTRL_ROW_SIZE];
        memcpy(page_buffer, (uint8_t *)CIRCUITPY_INTERNAL_CONFIG_START_ADDR, NVMCTRL_ROW_SIZE);

        // Modify it.
        memcpy(page_buffer, "CIRCUITPYTHON1", 15);
        // First 16 bytes (0-15) are ID. Little endian!
        page_buffer[16] = current_calibration & 0xff;
        page_buffer[17] = current_calibration >> 8;

        // Write it back.
        // We don't use features that use any advanced NVMCTRL features so we can fake the descriptor
        // whenever we need it instead of storing it long term.
        struct flash_descriptor desc;
        desc.dev.hw = NVMCTRL;
        flash_write(&desc, (uint32_t)CIRCUITPY_INTERNAL_CONFIG_START_ADDR, page_buffer, NVMCTRL_ROW_SIZE);
    }
}
#endif

static void rtc_continuous_mode(void) {
    #ifdef SAMD21
    while (RTC->MODE0.STATUS.bit.SYNCBUSY) {
    }
    RTC->MODE0.READREQ.reg = RTC_READREQ_RCONT | 0x0010;
    while (RTC->MODE0.STATUS.bit.SYNCBUSY) {
    }
    // Do the first request and wait for it.
    RTC->MODE0.READREQ.reg = RTC_READREQ_RREQ | RTC_READREQ_RCONT | 0x0010;
    while (RTC->MODE0.STATUS.bit.SYNCBUSY) {
    }
    #endif
}

static void rtc_init(void) {
    #ifdef SAMD21
    _gclk_enable_channel(RTC_GCLK_ID, GCLK_CLKCTRL_GEN_GCLK2_Val);
    RTC->MODE0.CTRL.bit.SWRST = true;
    while (RTC->MODE0.CTRL.bit.SWRST != 0) {
    }

    // Turn on periodic events to use as tick. We control whether it interrupts
    // us with the EVSYS INTEN register.
    RTC->MODE0.EVCTRL.reg = RTC_MODE0_EVCTRL_PEREO2;

    RTC->MODE0.CTRL.reg = RTC_MODE0_CTRL_ENABLE |
        RTC_MODE0_CTRL_MODE_COUNT32 |
        RTC_MODE0_CTRL_PRESCALER_DIV2;

    // Turn on continuous sync of the count register. This will speed up all
    // tick reads.
    rtc_continuous_mode();
    #endif
    #ifdef SAM_D5X_E5X
    hri_mclk_set_APBAMASK_RTC_bit(MCLK);
    #if CIRCUITPY_ALARM
    // Cache TAMPID for wake up cause
    (void)alarm_get_wakeup_cause();
    #endif
    RTC->MODE0.CTRLA.bit.SWRST = true;
    while (RTC->MODE0.SYNCBUSY.bit.SWRST != 0) {
    }

    RTC->MODE0.CTRLA.reg = RTC_MODE0_CTRLA_ENABLE |
        RTC_MODE0_CTRLA_MODE_COUNT32 |
        RTC_MODE0_CTRLA_PRESCALER_DIV2 |
        RTC_MODE0_CTRLA_COUNTSYNC;
    #endif


    // Set all peripheral interrupt priorities to the lowest priority by default.
    for (uint16_t i = 0; i < PERIPH_COUNT_IRQn; i++) {
        NVIC_SetPriority(i, (1UL << __NVIC_PRIO_BITS) - 1UL);
    }
    // Bump up the rtc interrupt so nothing else interferes with timekeeping.
    NVIC_SetPriority(RTC_IRQn, 0);
    #ifdef SAMD21
    NVIC_SetPriority(USB_IRQn, 1);
    #endif

    #ifdef SAM_D5X_E5X
    NVIC_SetPriority(USB_0_IRQn, 1);
    NVIC_SetPriority(USB_1_IRQn, 1);
    NVIC_SetPriority(USB_2_IRQn, 1);
    NVIC_SetPriority(USB_3_IRQn, 1);
    #endif
    NVIC_ClearPendingIRQ(RTC_IRQn);
    NVIC_EnableIRQ(RTC_IRQn);
    #if CIRCUITPY_RTC
    rtc_reset();
    #endif

}

safe_mode_t port_init(void) {
    #if defined(SAMD21)

    // Set brownout detection.
    // Disable while changing level.
    SYSCTRL->BOD33.bit.ENABLE = 0;
    SYSCTRL->BOD33.bit.LEVEL = SAMD21_BOD33_LEVEL;
    SYSCTRL->BOD33.bit.ENABLE = 1;

    #ifdef ENABLE_MICRO_TRACE_BUFFER
    REG_MTB_POSITION = ((uint32_t)(mtb - REG_MTB_BASE)) & 0xFFFFFFF8;
    REG_MTB_FLOW = (((uint32_t)mtb - REG_MTB_BASE) + TRACE_BUFFER_SIZE_BYTES) & 0xFFFFFFF8;
    REG_MTB_MASTER = 0x80000000 + (TRACE_BUFFER_MAGNITUDE_PACKETS - 1);
    #else
    // Triple check that the MTB is off. Switching between debug and non-debug
    // builds can leave it set over reset and wreak havok as a result.
    REG_MTB_MASTER = 0x00000000 + 6;
    #endif
    #endif

    #if defined(SAM_D5X_E5X)
    // Set brownout detection.
    // Disable while changing level.
    SUPC->BOD33.bit.ENABLE = 0;
    SUPC->BOD33.bit.LEVEL = SAMD5x_E5x_BOD33_LEVEL;
    SUPC->BOD33.bit.ENABLE = 1;

    // MPU (Memory Protection Unit) setup.
    // We hoped we could make the QSPI region be non-cachable with the MPU,
    // but the CMCC doesn't seem to pay attention to the MPU settings.
    // Leaving this code here disabled,
    // because it was hard enough to figure out, and maybe there's
    // a mistake that could make it work in the future.
    #if 0
    // Designate QSPI memory mapped region as not cacheable.

    // Turn off MPU in case it is on.
    MPU->CTRL = 0;
    // Configure region 0.
    MPU->RNR = 0;
    // Region base: start of QSPI mapping area.
    // QSPI region runs from 0x04000000 up to and not including 0x05000000: 16 megabytes
    MPU->RBAR = QSPI_AHB;
    MPU->RASR =
        0b011 << MPU_RASR_AP_Pos |     // full read/write access for privileged and user mode
            0b000 << MPU_RASR_TEX_Pos | // caching not allowed, strongly ordered
            1 << MPU_RASR_S_Pos |      // sharable
            0 << MPU_RASR_C_Pos |      // not cacheable
            0 << MPU_RASR_B_Pos |      // not bufferable
            0b10111 << MPU_RASR_SIZE_Pos | // 16MB region size
            1 << MPU_RASR_ENABLE_Pos   // enable this region
    ;
    // Turn off regions 1-7.
    for (uint32_t i = 1; i < 8; i++) {
        MPU->RNR = i;
        MPU->RBAR = 0;
        MPU->RASR = 0;
    }

    // Turn on MPU. Turn on PRIVDEFENA, which defines a default memory
    // map for all privileged access, so we don't have to set up other regions
    // besides QSPI.
    MPU->CTRL = MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk;
    #endif

    samd_peripherals_enable_cache();
    #endif

    #ifdef SAMD21
    hri_nvmctrl_set_CTRLB_RWS_bf(NVMCTRL, 2);
    _pm_init();
    #endif

    #if CALIBRATE_CRYSTALLESS
    uint32_t fine = DEFAULT_DFLL48M_FINE_CALIBRATION;
    // The fine calibration data is stored in an NVM page after the text and data storage but before
    // the optional file system. The first 16 bytes are the identifier for the section.
    if (strcmp((char *)CIRCUITPY_INTERNAL_CONFIG_START_ADDR, "CIRCUITPYTHON1") == 0) {
        fine = ((uint16_t *)CIRCUITPY_INTERNAL_CONFIG_START_ADDR)[8];
    }
    clock_init(BOARD_HAS_CRYSTAL, BOARD_XOSC_FREQ_HZ, BOARD_XOSC_IS_CRYSTAL, fine);
    #else
    // Use a default fine value
    clock_init(BOARD_HAS_CRYSTAL, BOARD_XOSC_FREQ_HZ, BOARD_XOSC_IS_CRYSTAL, DEFAULT_DFLL48M_FINE_CALIBRATION);
    #endif

    rtc_init();

    init_shared_dma();

    // Reset everything into a known state before board_init.
    reset_port();

    #ifdef SAMD21
    if (PM->RCAUSE.bit.BOD33 == 1 || PM->RCAUSE.bit.BOD12 == 1) {
        return SAFE_MODE_BROWNOUT;
    }
    #endif
    #ifdef SAM_D5X_E5X
    if (RSTC->RCAUSE.bit.BODVDD == 1 || RSTC->RCAUSE.bit.BODCORE == 1) {
        return SAFE_MODE_BROWNOUT;
    }
    #endif

    if (board_requests_safe_mode()) {
        return SAFE_MODE_USER;
    }

    return SAFE_MODE_NONE;
}

void reset_port(void) {
    #if CIRCUITPY_BUSIO
    reset_sercoms();
    #endif

    #if CIRCUITPY_AUDIOIO
    audio_dma_reset();
    #endif

    #if CIRCUITPY_AUDIOBUSIO
    pdmin_reset();
    #endif

    #if CIRCUITPY_AUDIOBUSIO_I2SOUT
    i2sout_reset();
    #endif

    #if CIRCUITPY_FREQUENCYIO
    frequencyin_reset();
    #endif

    #if CIRCUITPY_TOUCHIO && CIRCUITPY_TOUCHIO_USE_NATIVE
    touchin_reset();
    #endif

    eic_reset();

    #if CIRCUITPY_ANALOGIO
    analogin_reset();
    analogout_reset();
    #endif

    #if CIRCUITPY_WATCHDOG
    watchdog_reset();
    #endif

    reset_gclks();

    #if CIRCUITPY_PEW
    pew_reset();
    #endif

    #ifdef SAMD21
    if (!tick_enabled())
    // SAMD21 ticks depend on the event system, so don't disturb the event system if we need ticks,
    // such as for a display that lives across VM instantiations.
    #endif
    {
        reset_event_system();
        reset_ticks();
    }

    reset_all_pins();

    // Output clocks for debugging.
    // not supported by SAMD51G; uncomment for SAMD51J or update for 51G
    // #ifdef SAM_D5X_E5X
    // gpio_set_pin_function(PIN_PA10, GPIO_PIN_FUNCTION_M); // GCLK4, D3
    // gpio_set_pin_function(PIN_PA11, GPIO_PIN_FUNCTION_M); // GCLK5, A4
    // gpio_set_pin_function(PIN_PB14, GPIO_PIN_FUNCTION_M); // GCLK0, D5
    // gpio_set_pin_function(PIN_PB15, GPIO_PIN_FUNCTION_M); // GCLK1, D6
    // #endif

    #if CALIBRATE_CRYSTALLESS
    if (tud_cdc_connected()) {
        save_usb_clock_calibration();
    }
    #endif
}

void reset_to_bootloader(void) {
    _bootloader_dbl_tap = DBL_TAP_MAGIC;
    reset();
}

void reset_cpu(void) {
    reset();
}

uint32_t *port_stack_get_limit(void) {
    return port_stack_get_top() - (CIRCUITPY_DEFAULT_STACK_SIZE + CIRCUITPY_EXCEPTION_STACK_SIZE) / sizeof(uint32_t);
}

uint32_t *port_stack_get_top(void) {
    return &_estack;
}

// Used for the shared heap allocator.
uint32_t *port_heap_get_bottom(void) {
    return &_ebss;
}

uint32_t *port_heap_get_top(void) {
    return port_stack_get_limit();
}

// Place the word to save 8k from the end of RAM so we and the bootloader don't clobber it.
#ifdef SAMD21
uint32_t *safe_word = (uint32_t *)(HMCRAMC0_ADDR + HMCRAMC0_SIZE - 0x2000);
#endif
#ifdef SAM_D5X_E5X
uint32_t *safe_word = (uint32_t *)(HSRAM_ADDR + HSRAM_SIZE - 0x2000);
#endif

void port_set_saved_word(uint32_t value) {
    *safe_word = value;
}

uint32_t port_get_saved_word(void) {
    return *safe_word;
}

// TODO: Move this to an RTC backup register so we can preserve it when only the BACKUP power domain
// is enabled.
static volatile uint64_t overflowed_ticks = 0;
static uint32_t rtc_old_count;

static uint32_t _get_count(uint64_t *overflow_count) {
    #ifdef SAM_D5X_E5X
    while ((RTC->MODE0.SYNCBUSY.reg & (RTC_MODE0_SYNCBUSY_COUNTSYNC | RTC_MODE0_SYNCBUSY_COUNT)) != 0) {
    }
    #endif
    // SAMD21 does continuous sync so we don't need to wait here.

    uint32_t count = RTC->MODE0.COUNT.reg;
    if (count < rtc_old_count) {
        // Our RTC is 32 bits and we're clocking it at 16.384khz which is 16 (2 ** 4) subticks per
        // tick.
        overflowed_ticks += (1L << (32 - 4));
    }
    rtc_old_count = count;

    if (overflow_count != NULL) {
        *overflow_count = overflowed_ticks;
    }

    return count;
}

volatile bool _woken_up;

void RTC_Handler(void) {
    uint32_t intflag = RTC->MODE0.INTFLAG.reg;
    #ifdef SAM_D5X_E5X
    if (intflag & RTC_MODE0_INTFLAG_PER2) {
        RTC->MODE0.INTFLAG.reg = RTC_MODE0_INTFLAG_PER2;
        // Do things common to all ports when the tick occurs
        supervisor_tick();
    }
    #if CIRCUITPY_ALARM
    if (intflag & RTC_MODE0_INTFLAG_CMP1) {
        // Likely TimeAlarm fake sleep wake
        time_alarm_callback();
        RTC->MODE0.INTFLAG.reg = RTC_MODE0_INTFLAG_CMP1;
    }
    if (intflag & RTC_MODE0_INTFLAG_TAMPER) {
        // Likely PinAlarm fake sleep wake
        pin_alarm_callback(1); // TODO: set channel?
        RTC->MODE0.INTFLAG.reg = RTC_MODE0_INTFLAG_TAMPER;
    }
    #endif
    #endif
    if (intflag & RTC_MODE0_INTFLAG_CMP0) {
        // Clear the interrupt because we may have hit a sleep
        RTC->MODE0.INTFLAG.reg = RTC_MODE0_INTFLAG_CMP0;
        _woken_up = true;
        // SAMD21 ticks are handled by EVSYS
        #ifdef SAM_D5X_E5X
        RTC->MODE0.INTENCLR.reg = RTC_MODE0_INTENCLR_CMP0;
        #endif
    }
}

uint64_t port_get_raw_ticks(uint8_t *subticks) {
    uint64_t overflow_count;
    uint32_t current_ticks = _get_count(&overflow_count);
    if (subticks != NULL) {
        *subticks = (current_ticks % 16) * 2;
    }

    return overflow_count + current_ticks / 16;
}

static void evsyshandler_common(void) {
    #ifdef SAMD21
    if (_tick_event_channel < EVSYS_SYNCH_NUM && event_interrupt_active(_tick_event_channel)) {
        supervisor_tick();
    }
    #endif

    #if CIRCUITPY_AUDIOIO || CIRCUITPY_AUDIOBUSIO
    audio_dma_evsys_handler();
    #endif

    #if CIRCUITPY_AUDIOBUSIO
    pdmin_evsys_handler();
    #endif
}

#ifdef SAM_D5X_E5X
void EVSYS_0_Handler(void) {
    evsyshandler_common();
}
void EVSYS_1_Handler(void) {
    evsyshandler_common();
}
void EVSYS_2_Handler(void) {
    evsyshandler_common();
}
void EVSYS_3_Handler(void) {
    evsyshandler_common();
}
void EVSYS_4_Handler(void) {
    evsyshandler_common();
}
#else
void EVSYS_Handler(void) {
    evsyshandler_common();
}
#endif

// Enable 1/1024 second tick.
void port_enable_tick(void) {
    #ifdef SAM_D5X_E5X
    // PER2 will generate an interrupt every 32 ticks of the source 32.768 clock.
    RTC->MODE0.INTENSET.reg = RTC_MODE0_INTENSET_PER2;
    #endif
    #ifdef SAMD21
    // reset_port() preserves the event system if ticks were still enabled after a VM finished,
    // such as for an on-board display. Normally the event system would be reset between VM instantiations.
    if (_tick_event_channel >= EVSYS_SYNCH_NUM) {
        turn_on_event_system();
        _tick_event_channel = find_sync_event_channel();
    }
    // This turns on both the event detected interrupt (EVD) and overflow (OVR).
    init_event_channel_interrupt(_tick_event_channel, CORE_GCLK, EVSYS_ID_GEN_RTC_PER_2);
    // Disable overflow interrupt because we ignore it.
    if (_tick_event_channel >= 8) {
        uint8_t value = 1 << (_tick_event_channel - 8);
        EVSYS->INTENCLR.reg = EVSYS_INTENSET_OVRp8(value);
    } else {
        uint8_t value = 1 << _tick_event_channel;
        EVSYS->INTENCLR.reg = EVSYS_INTENSET_OVR(value);
    }
    NVIC_EnableIRQ(EVSYS_IRQn);
    #endif
}

// Disable 1/1024 second tick.
void port_disable_tick(void) {
    #ifdef SAM_D5X_E5X
    RTC->MODE0.INTENCLR.reg = RTC_MODE0_INTENCLR_PER2;
    #endif
    #ifdef SAMD21
    if (_tick_event_channel == EVSYS_SYNCH_NUM) {
        return;
    }

    if (_tick_event_channel >= 8) {
        uint8_t value = 1 << (_tick_event_channel - 8);
        EVSYS->INTENCLR.reg = EVSYS_INTENSET_EVDp8(value);
    } else {
        uint8_t value = 1 << _tick_event_channel;
        EVSYS->INTENCLR.reg = EVSYS_INTENSET_EVD(value);
    }
    disable_event_channel(_tick_event_channel);
    _tick_event_channel = EVSYS_SYNCH_NUM;
    #endif
}

void port_interrupt_after_ticks(uint32_t ticks) {
    uint32_t current_ticks = _get_count(NULL);
    if (ticks > 1 << 28) {
        // We'll interrupt sooner with an overflow.
        return;
    }
    #ifdef SAMD21
    if (sleep_disable_count > 0) {
        // "wake" immediately even if sleep_disable_count is set to 0 between
        // now and when port_idle_until_interrupt is called. Otherwise we may
        // sleep too long.
        _woken_up = true;
        return;
    }
    #endif

    uint32_t target = current_ticks + (ticks << 4);
    #ifdef SAMD21
    // Try and avoid a bus stall when writing COMP by checking for an obvious
    // existing sync.
    while (RTC->MODE0.STATUS.bit.SYNCBUSY == 1) {
    }
    #endif
    // Writing the COMP register can take up to 180us to synchronize. During
    // this time, the bus will stall and no interrupts will be serviced.
    RTC->MODE0.COMP[0].reg = target;
    #ifdef SAM_D5X_E5X
    while ((RTC->MODE0.SYNCBUSY.reg & (RTC_MODE0_SYNCBUSY_COMP0)) != 0) {
    }
    #endif
    RTC->MODE0.INTFLAG.reg = RTC_MODE0_INTFLAG_CMP0;
    RTC->MODE0.INTENSET.reg = RTC_MODE0_INTENSET_CMP0;
    // Set continuous mode again because setting COMP may disable it.
    rtc_continuous_mode();
    current_ticks = _get_count(NULL);
    _woken_up = current_ticks >= target;
}

void port_idle_until_interrupt(void) {
    #ifdef SAM_D5X_E5X
    // Clear the FPU interrupt because it can prevent us from sleeping.
    if (__get_FPSCR() & ~(0x9f)) {
        __set_FPSCR(__get_FPSCR() & ~(0x9f));
        (void)__get_FPSCR();
    }
    #endif
    common_hal_mcu_disable_interrupts();
    if (!background_callback_pending() && sleep_disable_count == 0 && !_woken_up) {
        __DSB();
        __WFI();
    }
    common_hal_mcu_enable_interrupts();
}

/**
 * \brief Default interrupt handler for unused IRQs.
 */
__attribute__((used)) void HardFault_Handler(void) {
    #ifdef ENABLE_MICRO_TRACE_BUFFER
    // Turn off the micro trace buffer so we don't fill it up in the infinite
    // loop below.
    REG_MTB_MASTER = 0x00000000 + 6;
    #endif

    reset_into_safe_mode(SAFE_MODE_HARD_FAULT);
    while (true) {
        asm ("nop;");
    }
}
