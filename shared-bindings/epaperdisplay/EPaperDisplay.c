// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/epaperdisplay/EPaperDisplay.h"

#include <stdint.h>

#include "shared/runtime/context_manager_helpers.h"
#include "py/binary.h"
#include "py/objproperty.h"
#include "py/objtype.h"
#include "py/runtime.h"
#include "shared-bindings/displayio/Group.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/util.h"
#include "shared-module/displayio/__init__.h"

//| from busdisplay import _DisplayBus
//|
//| class EPaperDisplay:
//|     """Manage updating an epaper display over a display bus
//|
//|     This initializes an epaper display and connects it into CircuitPython. Unlike other
//|     objects in CircuitPython, EPaperDisplay objects live until `displayio.release_displays()`
//|     is called. This is done so that CircuitPython can use the display itself.
//|
//|     Most people should not use this class directly. Use a specific display driver instead that will
//|     contain the startup and shutdown sequences at minimum."""
//|
//|     def __init__(
//|         self,
//|         display_bus: _DisplayBus,
//|         start_sequence: ReadableBuffer,
//|         stop_sequence: ReadableBuffer,
//|         *,
//|         width: int,
//|         height: int,
//|         ram_width: int,
//|         ram_height: int,
//|         colstart: int = 0,
//|         rowstart: int = 0,
//|         rotation: int = 0,
//|         set_column_window_command: Optional[int] = None,
//|         set_row_window_command: Optional[int] = None,
//|         set_current_column_command: Optional[int] = None,
//|         set_current_row_command: Optional[int] = None,
//|         write_black_ram_command: int,
//|         black_bits_inverted: bool = False,
//|         write_color_ram_command: Optional[int] = None,
//|         color_bits_inverted: bool = False,
//|         highlight_color: int = 0x000000,
//|         refresh_display_command: Union[int, circuitpython_typing.ReadableBuffer],
//|         refresh_time: float = 40,
//|         busy_pin: Optional[microcontroller.Pin] = None,
//|         busy_state: bool = True,
//|         seconds_per_frame: float = 180,
//|         always_toggle_chip_select: bool = False,
//|         grayscale: bool = False,
//|         advanced_color_epaper: bool = False,
//|         two_byte_sequence_length: bool = False,
//|         start_up_time: float = 0,
//|         address_little_endian: bool = False
//|     ) -> None:
//|         """Create a EPaperDisplay object on the given display bus (`fourwire.FourWire` or `paralleldisplaybus.ParallelBus`).
//|
//|         The ``start_sequence`` and ``stop_sequence`` are bitpacked to minimize the ram impact. Every
//|         command begins with a command byte followed by a byte to determine the parameter count and
//|         delay. When the top bit of the second byte is 1 (0x80), a delay will occur after the command
//|         parameters are sent. The remaining 7 bits are the parameter count excluding any delay
//|         byte. The bytes following are the parameters. When the delay bit is set, a single byte after
//|         the parameters specifies the delay duration in milliseconds. The value 0xff will lead to an
//|         extra long 500 ms delay instead of 255 ms. The next byte will begin a new command definition.
//|
//|         :param display_bus: The bus that the display is connected to
//|         :type _DisplayBus: fourwire.FourWire or paralleldisplaybus.ParallelBus
//|         :param ~circuitpython_typing.ReadableBuffer start_sequence: Byte-packed command sequence.
//|         :param ~circuitpython_typing.ReadableBuffer stop_sequence: Byte-packed command sequence.
//|         :param int width: Width in pixels
//|         :param int height: Height in pixels
//|         :param int ram_width: RAM width in pixels
//|         :param int ram_height: RAM height in pixels
//|         :param int colstart: The index if the first visible column
//|         :param int rowstart: The index if the first visible row
//|         :param int rotation: The rotation of the display in degrees clockwise. Must be in 90 degree increments (0, 90, 180, 270)
//|         :param int set_column_window_command: Command used to set the start and end columns to update
//|         :param int set_row_window_command: Command used so set the start and end rows to update
//|         :param int set_current_column_command: Command used to set the current column location
//|         :param int set_current_row_command: Command used to set the current row location
//|         :param int write_black_ram_command: Command used to write pixels values into the update region
//|         :param bool black_bits_inverted: True if 0 bits are used to show black pixels. Otherwise, 1 means to show black.
//|         :param int write_color_ram_command: Command used to write pixels values into the update region
//|         :param bool color_bits_inverted: True if 0 bits are used to show the color. Otherwise, 1 means to show color.
//|         :param int highlight_color: RGB888 of source color to highlight with third ePaper color.
//|         :param int refresh_display_command: Command used to start a display refresh. Single int or byte-packed command sequence
//|         :param float refresh_time: Time it takes to refresh the display before the stop_sequence should be sent. Ignored when busy_pin is provided.
//|         :param microcontroller.Pin busy_pin: Pin used to signify the display is busy
//|         :param bool busy_state: State of the busy pin when the display is busy
//|         :param float seconds_per_frame: Minimum number of seconds between screen refreshes
//|         :param bool always_toggle_chip_select: When True, chip select is toggled every byte
//|         :param bool grayscale: When true, the color ram is the low bit of 2-bit grayscale
//|         :param bool advanced_color_epaper: When true, the display is a 7-color advanced color epaper (ACeP)
//|         :param bool two_byte_sequence_length: When true, use two bytes to define sequence length
//|         :param float start_up_time: Time to wait after reset before sending commands
//|         :param bool address_little_endian: Send the least significant byte (not bit) of multi-byte addresses first. Ignored when ram is addressed with one byte
//|         """
//|         ...
static mp_obj_t epaperdisplay_epaperdisplay_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_display_bus, ARG_start_sequence, ARG_stop_sequence, ARG_width, ARG_height,
           ARG_ram_width, ARG_ram_height, ARG_colstart, ARG_rowstart, ARG_rotation,
           ARG_set_column_window_command, ARG_set_row_window_command, ARG_set_current_column_command,
           ARG_set_current_row_command, ARG_write_black_ram_command, ARG_black_bits_inverted,
           ARG_write_color_ram_command, ARG_color_bits_inverted, ARG_highlight_color,
           ARG_refresh_display_command,  ARG_refresh_time, ARG_busy_pin, ARG_busy_state,
           ARG_seconds_per_frame, ARG_always_toggle_chip_select, ARG_grayscale, ARG_advanced_color_epaper,
           ARG_two_byte_sequence_length, ARG_start_up_time, ARG_address_little_endian };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_display_bus, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_start_sequence, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_stop_sequence, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_width, MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, },
        { MP_QSTR_height, MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, },
        { MP_QSTR_ram_width, MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, },
        { MP_QSTR_ram_height, MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, },
        { MP_QSTR_colstart, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = 0} },
        { MP_QSTR_rowstart, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = 0} },
        { MP_QSTR_rotation, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = 0} },
        { MP_QSTR_set_column_window_command, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = NO_COMMAND} },
        { MP_QSTR_set_row_window_command, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = NO_COMMAND} },
        { MP_QSTR_set_current_column_command, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = NO_COMMAND} },
        { MP_QSTR_set_current_row_command, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = NO_COMMAND} },
        { MP_QSTR_write_black_ram_command, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_black_bits_inverted, MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool = false} },
        { MP_QSTR_write_color_ram_command, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_obj = mp_const_none} },
        { MP_QSTR_color_bits_inverted, MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool = false} },
        { MP_QSTR_highlight_color, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = 0x000000} },
        { MP_QSTR_refresh_display_command, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_refresh_time, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_obj = MP_OBJ_NEW_SMALL_INT(40)} },
        { MP_QSTR_busy_pin, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_obj = mp_const_none} },
        { MP_QSTR_busy_state, MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool = true} },
        { MP_QSTR_seconds_per_frame, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_obj = MP_OBJ_NEW_SMALL_INT(180)} },
        { MP_QSTR_always_toggle_chip_select, MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool = false} },
        { MP_QSTR_grayscale, MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool = false} },
        { MP_QSTR_advanced_color_epaper, MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool = false} },
        { MP_QSTR_two_byte_sequence_length, MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool = false} },
        { MP_QSTR_start_up_time, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_obj = MP_OBJ_NEW_SMALL_INT(0)} },
        { MP_QSTR_address_little_endian, MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool = false} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_t display_bus = args[ARG_display_bus].u_obj;

    mp_buffer_info_t start_bufinfo;
    mp_get_buffer_raise(args[ARG_start_sequence].u_obj, &start_bufinfo, MP_BUFFER_READ);
    mp_buffer_info_t stop_bufinfo;
    mp_get_buffer_raise(args[ARG_stop_sequence].u_obj, &stop_bufinfo, MP_BUFFER_READ);


    const mcu_pin_obj_t *busy_pin = validate_obj_is_free_pin_or_none(args[ARG_busy_pin].u_obj, MP_QSTR_busy_pin);

    mp_int_t rotation = args[ARG_rotation].u_int;
    if (rotation % 90 != 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("Display rotation must be in 90 degree increments"));
    }

    primary_display_t *disp = allocate_display_or_raise();
    epaperdisplay_epaperdisplay_obj_t *self = &disp->epaper_display;

    mp_float_t refresh_time = mp_obj_get_float(args[ARG_refresh_time].u_obj);
    mp_float_t seconds_per_frame = mp_obj_get_float(args[ARG_seconds_per_frame].u_obj);
    mp_float_t start_up_time = mp_obj_get_float(args[ARG_start_up_time].u_obj);

    mp_int_t write_color_ram_command = NO_COMMAND;
    mp_int_t highlight_color = args[ARG_highlight_color].u_int;
    if (args[ARG_write_color_ram_command].u_obj != mp_const_none) {
        write_color_ram_command = mp_obj_get_int(args[ARG_write_color_ram_command].u_obj);
    }

    bool two_byte_sequence_length = args[ARG_two_byte_sequence_length].u_bool;

    mp_obj_t refresh_obj = args[ARG_refresh_display_command].u_obj;
    const uint8_t *refresh_buf;
    mp_buffer_info_t refresh_bufinfo;
    size_t refresh_buf_len = 0;
    mp_int_t refresh_command;
    if (mp_obj_get_int_maybe(refresh_obj, &refresh_command)) {
        uint8_t *command_buf = m_malloc(3);
        command_buf[0] = refresh_command;
        command_buf[1] = 0;
        command_buf[2] = 0;
        refresh_buf = command_buf;
        refresh_buf_len = two_byte_sequence_length? 3: 2;
    } else if (mp_get_buffer(refresh_obj, &refresh_bufinfo, MP_BUFFER_READ)) {
        refresh_buf = refresh_bufinfo.buf;
        refresh_buf_len = refresh_bufinfo.len;
    } else {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("Invalid %q"), MP_QSTR_refresh_display_command);
    }

    self->base.type = &epaperdisplay_epaperdisplay_type;
    common_hal_epaperdisplay_epaperdisplay_construct(
        self,
        display_bus,
        start_bufinfo.buf, start_bufinfo.len, start_up_time, stop_bufinfo.buf, stop_bufinfo.len,
        args[ARG_width].u_int, args[ARG_height].u_int, args[ARG_ram_width].u_int, args[ARG_ram_height].u_int,
        args[ARG_colstart].u_int, args[ARG_rowstart].u_int, rotation,
        args[ARG_set_column_window_command].u_int, args[ARG_set_row_window_command].u_int,
        args[ARG_set_current_column_command].u_int, args[ARG_set_current_row_command].u_int,
        args[ARG_write_black_ram_command].u_int, args[ARG_black_bits_inverted].u_bool, write_color_ram_command,
        args[ARG_color_bits_inverted].u_bool, highlight_color, refresh_buf, refresh_buf_len, refresh_time,
        busy_pin, args[ARG_busy_state].u_bool, seconds_per_frame,
        args[ARG_always_toggle_chip_select].u_bool, args[ARG_grayscale].u_bool, args[ARG_advanced_color_epaper].u_bool,
        two_byte_sequence_length, args[ARG_address_little_endian].u_bool
        );

    return self;
}

// Helper to ensure we have the native super class instead of a subclass.
static epaperdisplay_epaperdisplay_obj_t *native_display(mp_obj_t display_obj) {
    mp_obj_t native_display = mp_obj_cast_to_native_base(display_obj, &epaperdisplay_epaperdisplay_type);
    mp_obj_assert_native_inited(native_display);
    return MP_OBJ_TO_PTR(native_display);
}

// Undocumented show() implementation with a friendly error message.
static mp_obj_t epaperdisplay_epaperdisplay_obj_show(mp_obj_t self_in, mp_obj_t group_in) {
    mp_raise_AttributeError(MP_ERROR_TEXT(".show(x) removed. Use .root_group = x"));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(epaperdisplay_epaperdisplay_show_obj, epaperdisplay_epaperdisplay_obj_show);

//|     def update_refresh_mode(
//|         self, start_sequence: ReadableBuffer, seconds_per_frame: float = 180
//|     ) -> None:
//|         """Updates the ``start_sequence`` and ``seconds_per_frame`` parameters to enable
//|         varying the refresh mode of the display."""
static mp_obj_t epaperdisplay_epaperdisplay_update_refresh_mode(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_start_sequence, ARG_seconds_per_frame };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_start_sequence, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_seconds_per_frame, MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_SMALL_INT(180)} },
    };
    epaperdisplay_epaperdisplay_obj_t *self = native_display(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get parameters
    mp_buffer_info_t start_sequence;
    mp_get_buffer_raise(args[ARG_start_sequence].u_obj, &start_sequence, MP_BUFFER_READ);
    float seconds_per_frame = mp_obj_get_float(args[ARG_seconds_per_frame].u_obj);

    // Update parameters
    epaperdisplay_epaperdisplay_change_refresh_mode_parameters(self, &start_sequence, seconds_per_frame);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(epaperdisplay_epaperdisplay_update_refresh_mode_obj, 1, epaperdisplay_epaperdisplay_update_refresh_mode);

//|     def refresh(self) -> None:
//|         """Refreshes the display immediately or raises an exception if too soon. Use
//|         ``time.sleep(display.time_to_refresh)`` to sleep until a refresh can occur."""
//|         ...
static mp_obj_t epaperdisplay_epaperdisplay_obj_refresh(mp_obj_t self_in) {
    epaperdisplay_epaperdisplay_obj_t *self = native_display(self_in);
    bool ok = common_hal_epaperdisplay_epaperdisplay_refresh(self);
    if (!ok) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Refresh too soon"));
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(epaperdisplay_epaperdisplay_refresh_obj, epaperdisplay_epaperdisplay_obj_refresh);

//|     time_to_refresh: float
//|     """Time, in fractional seconds, until the ePaper display can be refreshed."""
static mp_obj_t epaperdisplay_epaperdisplay_obj_get_time_to_refresh(mp_obj_t self_in) {
    epaperdisplay_epaperdisplay_obj_t *self = native_display(self_in);
    uint32_t refresh_ms = common_hal_epaperdisplay_epaperdisplay_get_time_to_refresh(self);
    if (refresh_ms == 0) {
        return mp_obj_new_float(0.0);
    }
    // Make sure non-zero values are always more than zero (the float conversion may round down.)
    return mp_obj_new_float((refresh_ms + 100) / 1000.0);
}
MP_DEFINE_CONST_FUN_OBJ_1(epaperdisplay_epaperdisplay_get_time_to_refresh_obj, epaperdisplay_epaperdisplay_obj_get_time_to_refresh);

MP_PROPERTY_GETTER(epaperdisplay_epaperdisplay_time_to_refresh_obj,
    (mp_obj_t)&epaperdisplay_epaperdisplay_get_time_to_refresh_obj);

//|     busy: bool
//|     """True when the display is refreshing. This uses the ``busy_pin`` when available or the
//|        ``refresh_time`` otherwise."""
static mp_obj_t epaperdisplay_epaperdisplay_obj_get_busy(mp_obj_t self_in) {
    epaperdisplay_epaperdisplay_obj_t *self = native_display(self_in);
    return mp_obj_new_bool(common_hal_epaperdisplay_epaperdisplay_get_busy(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(epaperdisplay_epaperdisplay_get_busy_obj, epaperdisplay_epaperdisplay_obj_get_busy);

MP_PROPERTY_GETTER(epaperdisplay_epaperdisplay_busy_obj,
    (mp_obj_t)&epaperdisplay_epaperdisplay_get_busy_obj);

//|     width: int
//|     """Gets the width of the display in pixels"""
static mp_obj_t epaperdisplay_epaperdisplay_obj_get_width(mp_obj_t self_in) {
    epaperdisplay_epaperdisplay_obj_t *self = native_display(self_in);
    return MP_OBJ_NEW_SMALL_INT(common_hal_epaperdisplay_epaperdisplay_get_width(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(epaperdisplay_epaperdisplay_get_width_obj, epaperdisplay_epaperdisplay_obj_get_width);

MP_PROPERTY_GETTER(epaperdisplay_epaperdisplay_width_obj,
    (mp_obj_t)&epaperdisplay_epaperdisplay_get_width_obj);

//|     height: int
//|     """Gets the height of the display in pixels"""
static mp_obj_t epaperdisplay_epaperdisplay_obj_get_height(mp_obj_t self_in) {
    epaperdisplay_epaperdisplay_obj_t *self = native_display(self_in);
    return MP_OBJ_NEW_SMALL_INT(common_hal_epaperdisplay_epaperdisplay_get_height(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(epaperdisplay_epaperdisplay_get_height_obj, epaperdisplay_epaperdisplay_obj_get_height);

MP_PROPERTY_GETTER(epaperdisplay_epaperdisplay_height_obj,
    (mp_obj_t)&epaperdisplay_epaperdisplay_get_height_obj);

//|     rotation: int
//|     """The rotation of the display as an int in degrees."""
static mp_obj_t epaperdisplay_epaperdisplay_obj_get_rotation(mp_obj_t self_in) {
    epaperdisplay_epaperdisplay_obj_t *self = native_display(self_in);
    return MP_OBJ_NEW_SMALL_INT(common_hal_epaperdisplay_epaperdisplay_get_rotation(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(epaperdisplay_epaperdisplay_get_rotation_obj, epaperdisplay_epaperdisplay_obj_get_rotation);
static mp_obj_t epaperdisplay_epaperdisplay_obj_set_rotation(mp_obj_t self_in, mp_obj_t value) {
    epaperdisplay_epaperdisplay_obj_t *self = native_display(self_in);
    common_hal_epaperdisplay_epaperdisplay_set_rotation(self, mp_obj_get_int(value));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(epaperdisplay_epaperdisplay_set_rotation_obj, epaperdisplay_epaperdisplay_obj_set_rotation);


MP_PROPERTY_GETSET(epaperdisplay_epaperdisplay_rotation_obj,
    (mp_obj_t)&epaperdisplay_epaperdisplay_get_rotation_obj,
    (mp_obj_t)&epaperdisplay_epaperdisplay_set_rotation_obj);

//|     bus: _DisplayBus
//|     """The bus being used by the display"""
//|
static mp_obj_t epaperdisplay_epaperdisplay_obj_get_bus(mp_obj_t self_in) {
    epaperdisplay_epaperdisplay_obj_t *self = native_display(self_in);
    return common_hal_epaperdisplay_epaperdisplay_get_bus(self);
}
MP_DEFINE_CONST_FUN_OBJ_1(epaperdisplay_epaperdisplay_get_bus_obj, epaperdisplay_epaperdisplay_obj_get_bus);

MP_PROPERTY_GETTER(epaperdisplay_epaperdisplay_bus_obj,
    (mp_obj_t)&epaperdisplay_epaperdisplay_get_bus_obj);

//|     root_group: displayio.Group
//|     """The root group on the epaper display.
//|     If the root group is set to `displayio.CIRCUITPYTHON_TERMINAL`, the default CircuitPython terminal will be shown.
//|     If the root group is set to ``None``, no output will be shown.
//|     """
//|
static mp_obj_t epaperdisplay_epaperdisplay_obj_get_root_group(mp_obj_t self_in) {
    epaperdisplay_epaperdisplay_obj_t *self = native_display(self_in);
    return common_hal_epaperdisplay_epaperdisplay_get_root_group(self);
}
MP_DEFINE_CONST_FUN_OBJ_1(epaperdisplay_epaperdisplay_get_root_group_obj, epaperdisplay_epaperdisplay_obj_get_root_group);

static mp_obj_t epaperdisplay_epaperdisplay_obj_set_root_group(mp_obj_t self_in, mp_obj_t group_in) {
    epaperdisplay_epaperdisplay_obj_t *self = native_display(self_in);
    displayio_group_t *group = NULL;
    if (group_in != mp_const_none) {
        group = MP_OBJ_TO_PTR(native_group(group_in));
    }

    common_hal_epaperdisplay_epaperdisplay_set_root_group(self, group);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(epaperdisplay_epaperdisplay_set_root_group_obj, epaperdisplay_epaperdisplay_obj_set_root_group);

MP_PROPERTY_GETSET(epaperdisplay_epaperdisplay_root_group_obj,
    (mp_obj_t)&epaperdisplay_epaperdisplay_get_root_group_obj,
    (mp_obj_t)&epaperdisplay_epaperdisplay_set_root_group_obj);

static const mp_rom_map_elem_t epaperdisplay_epaperdisplay_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_show), MP_ROM_PTR(&epaperdisplay_epaperdisplay_show_obj) },
    { MP_ROM_QSTR(MP_QSTR_update_refresh_mode), MP_ROM_PTR(&epaperdisplay_epaperdisplay_update_refresh_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_refresh), MP_ROM_PTR(&epaperdisplay_epaperdisplay_refresh_obj) },

    { MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&epaperdisplay_epaperdisplay_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_height), MP_ROM_PTR(&epaperdisplay_epaperdisplay_height_obj) },
    { MP_ROM_QSTR(MP_QSTR_rotation), MP_ROM_PTR(&epaperdisplay_epaperdisplay_rotation_obj) },
    { MP_ROM_QSTR(MP_QSTR_bus), MP_ROM_PTR(&epaperdisplay_epaperdisplay_bus_obj) },
    { MP_ROM_QSTR(MP_QSTR_busy), MP_ROM_PTR(&epaperdisplay_epaperdisplay_busy_obj) },
    { MP_ROM_QSTR(MP_QSTR_time_to_refresh), MP_ROM_PTR(&epaperdisplay_epaperdisplay_time_to_refresh_obj) },
    { MP_ROM_QSTR(MP_QSTR_root_group), MP_ROM_PTR(&epaperdisplay_epaperdisplay_root_group_obj) },
};
static MP_DEFINE_CONST_DICT(epaperdisplay_epaperdisplay_locals_dict, epaperdisplay_epaperdisplay_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    epaperdisplay_epaperdisplay_type,
    MP_QSTR_EPaperDisplay,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    make_new, epaperdisplay_epaperdisplay_make_new,
    locals_dict, &epaperdisplay_epaperdisplay_locals_dict
    );
