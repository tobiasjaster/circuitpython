// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Dan Halbert for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2018 Artur Pacholec
//
// SPDX-License-Identifier: MIT

#pragma once

#include "shared-module/_bleio/Attribute.h"
#include "common-hal/_bleio/Characteristic.h"
#include "common-hal/_bleio/Descriptor.h"
#include "common-hal/_bleio/UUID.h"

extern const mp_obj_type_t bleio_descriptor_type;

extern void common_hal_bleio_descriptor_construct(bleio_descriptor_obj_t *self, bleio_characteristic_obj_t *characteristic, bleio_uuid_obj_t *uuid, bleio_attribute_security_mode_t read_perm, bleio_attribute_security_mode_t write_perm, mp_int_t max_length, bool fixed_length, mp_buffer_info_t *initial_value_bufinfo);
extern bleio_uuid_obj_t *common_hal_bleio_descriptor_get_uuid(bleio_descriptor_obj_t *self);
extern bleio_characteristic_obj_t *common_hal_bleio_descriptor_get_characteristic(bleio_descriptor_obj_t *self);
extern size_t common_hal_bleio_descriptor_get_value(bleio_descriptor_obj_t *self, uint8_t *buf, size_t len);
extern void common_hal_bleio_descriptor_set_value(bleio_descriptor_obj_t *self, mp_buffer_info_t *bufinfo);
