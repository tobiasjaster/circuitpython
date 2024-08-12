// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2021 Mark Komus
//
// SPDX-License-Identifier: MIT

#pragma once

#include "shared-module/is31fl3741/IS31FL3741.h"

extern const mp_obj_type_t is31fl3741_IS31FL3741_type;

void common_hal_is31fl3741_IS31FL3741_construct(is31fl3741_IS31FL3741_obj_t *self, busio_i2c_obj_t *i2c, uint8_t addr);

void common_hal_is31fl3741_IS31FL3741_deinit(is31fl3741_IS31FL3741_obj_t *);

void common_hal_is31fl3741_write(is31fl3741_IS31FL3741_obj_t *is31, const mp_obj_t *mapping, const uint8_t *pixels, size_t numBytes);

void common_hal_is31fl3741_begin_transaction(is31fl3741_IS31FL3741_obj_t *self);
void common_hal_is31fl3741_end_transaction(is31fl3741_IS31FL3741_obj_t *self);

void common_hal_is31fl3741_send_unlock(is31fl3741_IS31FL3741_obj_t *self);
void common_hal_is31fl3741_set_page(is31fl3741_IS31FL3741_obj_t *self, uint8_t p);
void common_hal_is31fl3741_send_enable(is31fl3741_IS31FL3741_obj_t *self);
void common_hal_is31fl3741_send_reset(is31fl3741_IS31FL3741_obj_t *self);
void common_hal_is31fl3741_set_current(is31fl3741_IS31FL3741_obj_t *self, uint8_t current);
uint8_t common_hal_is31fl3741_get_current(is31fl3741_IS31FL3741_obj_t *self);
void common_hal_is31fl3741_set_led(is31fl3741_IS31FL3741_obj_t *self, uint16_t led, uint8_t level, uint8_t page);
void common_hal_is31fl3741_draw_pixel(is31fl3741_IS31FL3741_obj_t *self, int16_t x, int16_t y, uint32_t color, uint16_t *mapping, uint8_t display_height);
