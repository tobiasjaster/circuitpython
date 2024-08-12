// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Jeff Epler for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#include <stdint.h>
#include <stdbool.h>

/** @brief To be called once every ms
 *
 * The port must call supervisor_tick once per millisecond to perform regular tasks.
 * This is called from the SysTick interrupt or similar, and is safe to call in an
 * interrupt context.
 */
extern void supervisor_tick(void);

/** @brief Get the lower 32 bits of the time in milliseconds
 *
 * This can be more efficient than supervisor_ticks_ms64, for sites where a wraparound
 * of ~49.5 days is not harmful.
 */
extern uint32_t supervisor_ticks_ms32(void);

/** @brief Get the full time in milliseconds
 *
 * Because common ARM mcus cannot atomically work with 64-bit quantities, this
 * function must briefly disable interrupts in order to return the value.  If
 * only relative durations of less than about ~49.5 days need to be considered,
 * then it may be possible to use supervisor_ticks_ms32() instead.
 */
extern uint64_t supervisor_ticks_ms64(void);

extern void supervisor_enable_tick(void);
extern void supervisor_disable_tick(void);

/**
 * @brief Return true if tick-based background tasks ran within the last 1s
 *
 * Note that when ticks are not enabled, this function can return false; this is
 * intended.
 */
extern bool supervisor_background_ticks_ok(void);
