// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2018 hathach for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "lib/tinyusb/src/tusb.h"

#include "py/objstr.h"
#include "py/runtime.h"
#include "supervisor/port.h"
#include "supervisor/shared/safe_mode.h"
#include "supervisor/usb.h"

#if CIRCUITPY_USB_CDC
#include "shared-bindings/usb_cdc/__init__.h"
#endif

#if CIRCUITPY_USB_HID
#include "shared-bindings/usb_hid/__init__.h"
#endif

#if CIRCUITPY_USB_MIDI
#include "shared-bindings/usb_midi/__init__.h"
#endif

#if CIRCUITPY_USB_MSC && CIRCUITPY_STORAGE
#include "shared-bindings/storage/__init__.h"
#endif

#if CIRCUITPY_USB_VIDEO
#include "shared-module/usb_video/__init__.h"
#endif

#include "shared-bindings/microcontroller/Processor.h"


// Table for collecting interface strings (interface names) as descriptor is built.
// We reuse the same table after collection, replacing the char string pointers with le16 string pointers.
#define MAX_INTERFACE_STRINGS 16
// slot 0 is always the Language ID
typedef union {
    const char *char_str;
    const uint16_t *descriptor;
} interface_string_t;
static interface_string_t collected_interface_strings[MAX_INTERFACE_STRINGS];

static size_t collected_interface_strings_length;
static uint8_t current_interface_string;

static uint8_t *device_descriptor;
static uint8_t *configuration_descriptor;
static uint16_t *string_descriptors;

// Serial number string is UID length * 2 (2 nibbles per byte) + 1 byte for null termination.
static char serial_number_hex_string[COMMON_HAL_MCU_PROCESSOR_UID_LENGTH * 2 + 1];

static const uint8_t device_descriptor_template[] = {
    0x12,        //  0 bLength
    0x01,        //  1 bDescriptorType (Device)
    0x00, 0x02,  //  2,3 bcdUSB 2.00
    0x00,        //  4 bDeviceClass (Use class information in the Interface Descriptors)
    0x00,        //  5 bDeviceSubClass
    0x00,        //  6 bDeviceProtocol
    0x40,        //  7 bMaxPacketSize0 64
    0xFF, 0xFF,  //  8,9 idVendor [SET AT RUNTIME: lo,hi]
#define DEVICE_VID_LO_INDEX (8)
#define DEVICE_VID_HI_INDEX (9)
    0xFF, 0xFF,  // 10,11 idProduct [SET AT RUNTIME: lo,hi]
#define DEVICE_PID_LO_INDEX (10)
#define DEVICE_PID_HI_INDEX (11)
    0x00, 0x01,  // 12,13 bcdDevice 2.00
    0xFF,        // 14 iManufacturer (String Index) [SET AT RUNTIME]
#define DEVICE_MANUFACTURER_STRING_INDEX (14)
    0xFF,        // 15 iProduct (String Index) [SET AT RUNTIME]
#define DEVICE_PRODUCT_STRING_INDEX (15)
    0xFF,        // 16 iSerialNumber (String Index)  [SET AT RUNTIME]
#define DEVICE_SERIAL_NUMBER_STRING_INDEX (16)
    0x01,        // 17 bNumConfigurations 1
};

static const uint8_t configuration_descriptor_template[] = {
    0x09,        // 0 bLength
    0x02,        // 1 bDescriptorType (Configuration)
    0xFF, 0xFF,  // 2,3 wTotalLength  [SET AT RUNTIME: lo, hi]
#define CONFIG_TOTAL_LENGTH_LO_INDEX (2)
#define CONFIG_TOTAL_LENGTH_HI_INDEX (3)
    0xFF,        // 4 bNumInterfaces  [SET AT RUNTIME]
#define CONFIG_NUM_INTERFACES_INDEX (4)
    0x01,        // 5 bConfigurationValue
    0x00,        // 6 iConfiguration (String Index)
    0x80 | TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,        // 7 bmAttributes
    0x32,        // 8 bMaxPower 100mA
};

static bool usb_build_device_descriptor(const usb_identification_t *identification) {
    device_descriptor =
        (uint8_t *)port_malloc(sizeof(device_descriptor_template),
            /*dma_capable*/ false);
    if (device_descriptor == NULL) {
        return false;
    }
    memcpy(device_descriptor, device_descriptor_template, sizeof(device_descriptor_template));

    device_descriptor[DEVICE_VID_LO_INDEX] = identification->vid & 0xFF;
    device_descriptor[DEVICE_VID_HI_INDEX] = identification->vid >> 8;
    device_descriptor[DEVICE_PID_LO_INDEX] = identification->pid & 0xFF;
    device_descriptor[DEVICE_PID_HI_INDEX] = identification->pid >> 8;

    usb_add_interface_string(current_interface_string, identification->manufacturer_name);
    device_descriptor[DEVICE_MANUFACTURER_STRING_INDEX] = current_interface_string;
    current_interface_string++;

    usb_add_interface_string(current_interface_string, identification->product_name);
    device_descriptor[DEVICE_PRODUCT_STRING_INDEX] = current_interface_string;
    current_interface_string++;

    usb_add_interface_string(current_interface_string, serial_number_hex_string);
    device_descriptor[DEVICE_SERIAL_NUMBER_STRING_INDEX] = current_interface_string;
    current_interface_string++;

    return true;
}

static bool usb_build_configuration_descriptor(void) {
    size_t total_descriptor_length = sizeof(configuration_descriptor_template);

    // CDC should be first, for compatibility with Adafruit Windows 7 drivers.
    // In the past, the order has been CDC, MSC, MIDI, HID, so preserve that order.
    #if CIRCUITPY_USB_CDC
    if (usb_cdc_console_enabled()) {
        total_descriptor_length += usb_cdc_descriptor_length();
    }
    if (usb_cdc_data_enabled()) {
        total_descriptor_length += usb_cdc_descriptor_length();
    }
    #endif

    #if CIRCUITPY_USB_MSC
    if (storage_usb_enabled()) {
        total_descriptor_length += storage_usb_descriptor_length();
    }
    #endif

    #if CIRCUITPY_USB_HID
    if (usb_hid_enabled()) {
        total_descriptor_length += usb_hid_descriptor_length();
    }
    #endif

    #if CIRCUITPY_USB_MIDI
    if (usb_midi_enabled()) {
        total_descriptor_length += usb_midi_descriptor_length();
    }
    #endif

    #if CIRCUITPY_USB_VENDOR
    if (usb_vendor_enabled()) {
        total_descriptor_length += usb_vendor_descriptor_length();
    }
    #endif

    #if CIRCUITPY_USB_VIDEO
    if (usb_video_enabled()) {
        total_descriptor_length += usb_video_descriptor_length();
    }
    #endif

    // Now we know how big the configuration descriptor will be, so we can allocate space for it.
    configuration_descriptor =
        (uint8_t *)port_malloc(total_descriptor_length,
            /*dma_capable*/ false);
    if (configuration_descriptor == NULL) {
        return false;
    }

    // Copy the template, which is the first part of the descriptor, and fix up its length.

    memcpy(configuration_descriptor, configuration_descriptor_template, sizeof(configuration_descriptor_template));

    configuration_descriptor[CONFIG_TOTAL_LENGTH_LO_INDEX] = total_descriptor_length & 0xFF;
    configuration_descriptor[CONFIG_TOTAL_LENGTH_HI_INDEX] = (total_descriptor_length >> 8) & 0xFF;

    // Number interfaces and endpoints.
    // Endpoint 0 is already used for USB control,
    // so start with 1 for the current endpoint and for the number of in and out endpoints
    // already in use.

    descriptor_counts_t descriptor_counts = {
        .current_interface = 0,
        .current_endpoint = 1,
        .num_in_endpoints = 1,
        .num_out_endpoints = 1,
    };

    uint8_t *descriptor_buf_remaining = configuration_descriptor + sizeof(configuration_descriptor_template);

    #if CIRCUITPY_USB_CDC
    if (usb_cdc_console_enabled()) {
        // Concatenate and fix up the CDC REPL descriptor.
        descriptor_buf_remaining += usb_cdc_add_descriptor(
            descriptor_buf_remaining, &descriptor_counts, &current_interface_string, true /*console*/);

    }
    if (usb_cdc_data_enabled()) {
        // Concatenate and fix up the CDC data descriptor.
        descriptor_buf_remaining += usb_cdc_add_descriptor(
            descriptor_buf_remaining, &descriptor_counts, &current_interface_string, false /*console*/);
    }
    #endif

    #if CIRCUITPY_USB_MSC
    if (storage_usb_enabled()) {
        // Concatenate and fix up the MSC descriptor.
        descriptor_buf_remaining += storage_usb_add_descriptor(
            descriptor_buf_remaining, &descriptor_counts, &current_interface_string);
    }
    #endif

    #if CIRCUITPY_USB_HID
    if (usb_hid_enabled()) {
        if (usb_hid_boot_device() > 0 && descriptor_counts.current_interface > 0) {
            // Hosts using boot devices generally to expect them to be at interface zero,
            // and will not work properly otherwise.
            reset_into_safe_mode(SAFE_MODE_USB_BOOT_DEVICE_NOT_INTERFACE_ZERO);
        }
        descriptor_buf_remaining += usb_hid_add_descriptor(
            descriptor_buf_remaining, &descriptor_counts, &current_interface_string,
            usb_hid_report_descriptor_length(), usb_hid_boot_device());
    }
    #endif

    #if CIRCUITPY_USB_MIDI
    if (usb_midi_enabled()) {
        // Concatenate and fix up the MIDI descriptor.
        descriptor_buf_remaining += usb_midi_add_descriptor(
            descriptor_buf_remaining, &descriptor_counts, &current_interface_string);
    }
    #endif

    #if CIRCUITPY_USB_VENDOR
    if (usb_vendor_enabled()) {
        descriptor_buf_remaining += usb_vendor_add_descriptor(
            descriptor_buf_remaining, &descriptor_counts, &current_interface_string);
    }
    #endif

    #if CIRCUITPY_USB_VIDEO
    if (usb_video_enabled()) {
        descriptor_buf_remaining += usb_video_add_descriptor(
            descriptor_buf_remaining, &descriptor_counts, &current_interface_string);
    }
    #endif
    // Now we know how many interfaces have been used.
    configuration_descriptor[CONFIG_NUM_INTERFACES_INDEX] = descriptor_counts.current_interface;

    // Did we run out of endpoints?
    if (descriptor_counts.current_endpoint > USB_NUM_ENDPOINT_PAIRS ||
        descriptor_counts.num_in_endpoints > USB_NUM_IN_ENDPOINTS ||
        descriptor_counts.num_out_endpoints > USB_NUM_OUT_ENDPOINTS) {
        reset_into_safe_mode(SAFE_MODE_USB_TOO_MANY_ENDPOINTS);
    }
    return true;
}

// str must not be on the heap.
void usb_add_interface_string(uint8_t interface_string_index, const char str[]) {
    if (interface_string_index > MAX_INTERFACE_STRINGS) {
        reset_into_safe_mode(SAFE_MODE_USB_TOO_MANY_INTERFACE_NAMES);
    }

    collected_interface_strings[interface_string_index].char_str = str;
    collected_interface_strings_length += strlen(str);
}

static const uint16_t language_id[] = {
    0x0304,
    0x0409,
};

static bool usb_build_interface_string_table(void) {
    // Allocate space for the le16 String descriptors.
    // Space needed is 2 bytes for String Descriptor header, then 2 bytes for each character
    string_descriptors =
        port_malloc(current_interface_string * 2 + collected_interface_strings_length * 2,
            /*dma_capable*/ false);
    if (string_descriptors == NULL) {
        return false;
    }

    uint16_t *string_descriptor = string_descriptors;

    // Language ID is always the 0th string descriptor.
    collected_interface_strings[0].descriptor = language_id;

    // Build the le16 versions of all the descriptor strings.
    // Start at 1 to skip the Language ID.
    for (uint8_t string_index = 1; string_index < current_interface_string; string_index++) {
        const char *str = collected_interface_strings[string_index].char_str;
        const size_t str_len = strlen(str);
        // 1 word for descriptor type and length, 1 word for each character.
        const uint8_t descriptor_size_words = 1 + str_len;
        const uint8_t descriptor_size_bytes = descriptor_size_words * 2;
        string_descriptor[0] = 0x0300 | descriptor_size_bytes;

        // Convert to le16.
        for (size_t i = 0; i < str_len; i++) {
            string_descriptor[i + 1] = str[i];
        }

        // Save ptr to string descriptor with le16 str.
        collected_interface_strings[string_index].descriptor = string_descriptor;

        // Move to next descriptor slot.
        string_descriptor += descriptor_size_words;
    }
    return true;
}

// After boot.py runs, the USB devices to be used have been chosen, and the descriptors can be set up.
bool usb_build_descriptors(const usb_identification_t *identification) {
    uint8_t raw_id[COMMON_HAL_MCU_PROCESSOR_UID_LENGTH];
    common_hal_mcu_processor_get_uid(raw_id);

    for (int i = 0; i < COMMON_HAL_MCU_PROCESSOR_UID_LENGTH; i++) {
        for (int j = 0; j < 2; j++) {
            uint8_t nibble = (raw_id[i] >> (j * 4)) & 0xf;
            serial_number_hex_string[i * 2 + (1 - j)] = nibble_to_hex_upper[nibble];
        }
    }

    // Null-terminate the string.
    serial_number_hex_string[sizeof(serial_number_hex_string) - 1] = '\0';

    current_interface_string = 1;
    collected_interface_strings_length = 0;

    return usb_build_device_descriptor(identification) &&
           usb_build_configuration_descriptor() &&
           usb_build_interface_string_table();
}

// Invoked when GET DEVICE DESCRIPTOR is received.
// Application return pointer to descriptor
uint8_t const *tud_descriptor_device_cb(void) {
    return device_descriptor;
}

// Invoked when GET CONFIGURATION DESCRIPTOR is received.
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;  // for multiple configurations
    return configuration_descriptor;
}

// Invoked when GET STRING DESCRIPTOR request is received.
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    if (index > MAX_INTERFACE_STRINGS) {
        return NULL;
    }
    return collected_interface_strings[index].descriptor;
}
