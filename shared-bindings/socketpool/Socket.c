// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Scott Shawcroft for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2021 Lucian Copeland for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/socketpool/Socket.h"

#include <stdio.h>
#include <string.h>

#include "py/mperrno.h"
#include "py/objlist.h"
#include "py/objproperty.h"
#include "py/objtuple.h"
#include "py/runtime.h"
#include "py/stream.h"

#include "shared/netutils/netutils.h"
#include "shared/runtime/context_manager_helpers.h"
#include "shared/runtime/interrupt_char.h"

//| class Socket:
//|     """TCP, UDP and RAW socket. Cannot be created directly. Instead, call
//|     `SocketPool.socket()`.
//|
//|     Provides a subset of CPython's `socket.socket` API. It only implements the versions of
//|     recv that do not allocate bytes objects."""
//|

//|     def __hash__(self) -> int:
//|         """Returns a hash for the Socket."""
//|         ...
// Provided inherently.
// See https://github.com/micropython/micropython/pull/10348.

//|     def __enter__(self) -> Socket:
//|         """No-op used by Context Managers."""
//|         ...
//  Provided by context manager helper.

//|     def __exit__(self) -> None:
//|         """Automatically closes the Socket when exiting a context. See
//|         :ref:`lifetime-and-contextmanagers` for more info."""
//|         ...
static mp_obj_t socketpool_socket___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    common_hal_socketpool_socket_close(args[0]);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(socketpool_socket___exit___obj, 4, 4, socketpool_socket___exit__);

//|     def accept(self) -> Tuple[Socket, Tuple[str, int]]:
//|         """Accept a connection on a listening socket of type SOCK_STREAM,
//|         creating a new socket of type SOCK_STREAM.
//|         Returns a tuple of (new_socket, remote_address)"""
static mp_obj_t _socketpool_socket_accept(mp_obj_t self_in) {
    socketpool_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_obj_t tuple_contents[2];
    tuple_contents[0] = MP_OBJ_FROM_PTR(common_hal_socketpool_socket_accept(self, &tuple_contents[1]));

    return mp_obj_new_tuple(2, tuple_contents);
}
static MP_DEFINE_CONST_FUN_OBJ_1(socketpool_socket_accept_obj, _socketpool_socket_accept);

//|     def bind(self, address: Tuple[str, int]) -> None:
//|         """Bind a socket to an address
//|
//|         :param ~tuple address: tuple of (remote_address, remote_port)"""
//|         ...
static mp_obj_t socketpool_socket_bind(mp_obj_t self_in, mp_obj_t addr_in) {
    socketpool_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_obj_t *addr_items;
    mp_obj_get_array_fixed_n(addr_in, 2, &addr_items);

    size_t hostlen;
    const char *host = mp_obj_str_get_data(addr_items[0], &hostlen);
    mp_int_t port = mp_obj_get_int(addr_items[1]);
    if (port < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("port must be >= 0"));
    }

    size_t error = common_hal_socketpool_socket_bind(self, host, hostlen, (uint32_t)port);
    if (error != 0) {
        mp_raise_OSError(error);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(socketpool_socket_bind_obj, socketpool_socket_bind);

//|     def close(self) -> None:
//|         """Closes this Socket and makes its resources available to its SocketPool."""
static mp_obj_t _socketpool_socket_close(mp_obj_t self_in) {
    socketpool_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_socketpool_socket_close(self);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(socketpool_socket_close_obj, _socketpool_socket_close);

//|     def connect(self, address: Tuple[str, int]) -> None:
//|         """Connect a socket to a remote address
//|
//|         :param ~tuple address: tuple of (remote_address, remote_port)"""
//|         ...
static mp_obj_t socketpool_socket_connect(mp_obj_t self_in, mp_obj_t addr_in) {
    socketpool_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_obj_t *addr_items;
    mp_obj_get_array_fixed_n(addr_in, 2, &addr_items);

    size_t hostlen;
    const char *host = mp_obj_str_get_data(addr_items[0], &hostlen);
    mp_int_t port = mp_obj_get_int(addr_items[1]);
    if (port < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("port must be >= 0"));
    }

    common_hal_socketpool_socket_connect(self, host, hostlen, (uint32_t)port);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(socketpool_socket_connect_obj, socketpool_socket_connect);

//|     def listen(self, backlog: int) -> None:
//|         """Set socket to listen for incoming connections
//|
//|         :param ~int backlog: length of backlog queue for waiting connections"""
//|         ...
static mp_obj_t socketpool_socket_listen(mp_obj_t self_in, mp_obj_t backlog_in) {
    socketpool_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int backlog = mp_obj_get_int(backlog_in);

    common_hal_socketpool_socket_listen(self, backlog);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(socketpool_socket_listen_obj, socketpool_socket_listen);

//|     def recvfrom_into(self, buffer: WriteableBuffer) -> Tuple[int, Tuple[str, int]]:
//|         """Reads some bytes from a remote address.
//|
//|         Returns a tuple containing
//|         * the number of bytes received into the given buffer
//|         * a remote_address, which is a tuple of ip address and port number
//|
//|         :param object buffer: buffer to read into"""
//|         ...
static mp_obj_t socketpool_socket_recvfrom_into(mp_obj_t self_in, mp_obj_t data_in) {
    socketpool_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(data_in, &bufinfo, MP_BUFFER_WRITE);

    mp_obj_t tuple_contents[2];
    tuple_contents[0] = mp_obj_new_int_from_uint(common_hal_socketpool_socket_recvfrom_into(self,
        (byte *)bufinfo.buf, bufinfo.len, &tuple_contents[1]));
    return mp_obj_new_tuple(2, tuple_contents);
}
static MP_DEFINE_CONST_FUN_OBJ_2(socketpool_socket_recvfrom_into_obj, socketpool_socket_recvfrom_into);

//|     def recv_into(self, buffer: WriteableBuffer, bufsize: int) -> int:
//|         """Reads some bytes from the connected remote address, writing
//|         into the provided buffer. If bufsize <= len(buffer) is given,
//|         a maximum of bufsize bytes will be read into the buffer. If no
//|         valid value is given for bufsize, the default is the length of
//|         the given buffer.
//|
//|         Suits sockets of type SOCK_STREAM
//|         Returns an int of number of bytes read.
//|
//|         :param bytearray buffer: buffer to receive into
//|         :param int bufsize: optionally, a maximum number of bytes to read."""
//|         ...
static mp_obj_t _socketpool_socket_recv_into(size_t n_args, const mp_obj_t *args) {
    socketpool_socket_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (common_hal_socketpool_socket_get_closed(self)) {
        // Bad file number.
        mp_raise_OSError(MP_EBADF);
    }
    // if (!common_hal_socketpool_socket_get_connected(self)) {
    //     // not connected
    //     mp_raise_OSError(MP_ENOTCONN);
    // }
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_WRITE);
    mp_int_t len = bufinfo.len;
    if (n_args == 3) {
        mp_int_t given_len = mp_obj_get_int(args[2]);
        if (given_len > len) {
            mp_raise_ValueError(MP_ERROR_TEXT("buffer too small for requested bytes"));
        }
        if (given_len > 0 && given_len < len) {
            len = given_len;
        }
    }

    if (len == 0) {
        return MP_OBJ_NEW_SMALL_INT(0);
    }

    mp_int_t ret = common_hal_socketpool_socket_recv_into(self, (byte *)bufinfo.buf, len);
    return mp_obj_new_int_from_uint(ret);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(socketpool_socket_recv_into_obj, 2, 3, _socketpool_socket_recv_into);

//|     def send(self, bytes: ReadableBuffer) -> int:
//|         """Send some bytes to the connected remote address.
//|         Suits sockets of type SOCK_STREAM
//|
//|         :param ~bytes bytes: some bytes to send"""
//|         ...
static mp_obj_t _socketpool_socket_send(mp_obj_t self_in, mp_obj_t buf_in) {
    socketpool_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (common_hal_socketpool_socket_get_closed(self)) {
        // Bad file number.
        mp_raise_OSError(MP_EBADF);
    }
    if (!common_hal_socketpool_socket_get_connected(self)) {
        mp_raise_BrokenPipeError();
    }
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);
    mp_int_t ret = common_hal_socketpool_socket_send(self, bufinfo.buf, bufinfo.len);
    if (ret == -1) {
        mp_raise_BrokenPipeError();
    }
    return mp_obj_new_int_from_uint(ret);
}
static MP_DEFINE_CONST_FUN_OBJ_2(socketpool_socket_send_obj, _socketpool_socket_send);

//|     def sendall(self, bytes: ReadableBuffer) -> None:
//|         """Send some bytes to the connected remote address.
//|         Suits sockets of type SOCK_STREAM
//|
//|         This calls send() repeatedly until all the data is sent or an error
//|         occurs. If an error occurs, it's impossible to tell how much data
//|         has been sent.
//|
//|         :param ~bytes bytes: some bytes to send"""
//|         ...
static mp_obj_t _socketpool_socket_sendall(mp_obj_t self_in, mp_obj_t buf_in) {
    socketpool_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (common_hal_socketpool_socket_get_closed(self)) {
        // Bad file number.
        mp_raise_OSError(MP_EBADF);
    }
    if (!common_hal_socketpool_socket_get_connected(self)) {
        mp_raise_BrokenPipeError();
    }
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);
    while (bufinfo.len > 0) {
        mp_int_t ret = common_hal_socketpool_socket_send(self, bufinfo.buf, bufinfo.len);
        if (ret == -1) {
            mp_raise_BrokenPipeError();
        }
        bufinfo.len -= ret;
        bufinfo.buf += ret;
        if (bufinfo.len > 0) {
            RUN_BACKGROUND_TASKS;
            // Allow user to break out of sendall with a KeyboardInterrupt.
            if (mp_hal_is_interrupted()) {
                return 0;
            }
        }
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(socketpool_socket_sendall_obj, _socketpool_socket_sendall);

//|     def sendto(self, bytes: ReadableBuffer, address: Tuple[str, int]) -> int:
//|         """Send some bytes to a specific address.
//|         Suits sockets of type SOCK_DGRAM
//|
//|         :param ~bytes bytes: some bytes to send
//|         :param ~tuple address: tuple of (remote_address, remote_port)"""
//|         ...
static mp_obj_t socketpool_socket_sendto(mp_obj_t self_in, mp_obj_t data_in, mp_obj_t addr_in) {
    socketpool_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // get the data
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(data_in, &bufinfo, MP_BUFFER_READ);

    mp_obj_t *addr_items;
    mp_obj_get_array_fixed_n(addr_in, 2, &addr_items);

    size_t hostlen;
    const char *host = mp_obj_str_get_data(addr_items[0], &hostlen);
    mp_int_t port = mp_obj_get_int(addr_items[1]);
    if (port < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("port must be >= 0"));
    }

    mp_int_t ret = common_hal_socketpool_socket_sendto(self, host, hostlen, (uint32_t)port, bufinfo.buf, bufinfo.len);

    return mp_obj_new_int_from_uint(ret);
}
static MP_DEFINE_CONST_FUN_OBJ_3(socketpool_socket_sendto_obj, socketpool_socket_sendto);

//|     def setblocking(self, flag: bool) -> Optional[int]:
//|         """Set the blocking behaviour of this socket.
//|
//|         :param ~bool flag: False means non-blocking, True means block indefinitely."""
//|         ...
// method socket.setblocking(flag)
static mp_obj_t socketpool_socket_setblocking(mp_obj_t self_in, mp_obj_t blocking) {
    socketpool_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (mp_obj_is_true(blocking)) {
        common_hal_socketpool_socket_settimeout(self, -1);
    } else {
        common_hal_socketpool_socket_settimeout(self, 0);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(socketpool_socket_setblocking_obj, socketpool_socket_setblocking);

//|     def setsockopt(self, level: int, optname: int, value: int) -> None:
//|         """Sets socket options"""
//|         ...
static mp_obj_t socketpool_socket_setsockopt(size_t n_args, const mp_obj_t *args) {
    socketpool_socket_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t level = mp_obj_get_int(args[1]);
    mp_int_t opt = mp_obj_get_int(args[2]);

    const void *optval;
    mp_uint_t optlen;
    mp_int_t val;
    if (mp_obj_is_integer(args[3])) {
        val = mp_obj_get_int_truncated(args[3]);
        optval = &val;
        optlen = sizeof(val);
    } else {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[3], &bufinfo, MP_BUFFER_READ);
        optval = bufinfo.buf;
        optlen = bufinfo.len;
    }

    int _errno = common_hal_socketpool_socket_setsockopt(self, level, opt, optval, optlen);
    if (_errno < 0) {
        mp_raise_OSError(-_errno);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(socketpool_socket_setsockopt_obj, 4, 4, socketpool_socket_setsockopt);


//|     def settimeout(self, value: int) -> None:
//|         """Set the timeout value for this socket.
//|
//|         :param ~int value: timeout in seconds.  0 means non-blocking.  None means block indefinitely.
//|         """
//|         ...
static mp_obj_t socketpool_socket_settimeout(mp_obj_t self_in, mp_obj_t timeout_in) {
    socketpool_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_uint_t timeout_ms;
    if (timeout_in == mp_const_none) {
        timeout_ms = -1;
    } else {
        #if MICROPY_PY_BUILTINS_FLOAT
        timeout_ms = 1000 * mp_obj_get_float(timeout_in);
        #else
        timeout_ms = 1000 * mp_obj_get_int(timeout_in);
        #endif
    }
    common_hal_socketpool_socket_settimeout(self, timeout_ms);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(socketpool_socket_settimeout_obj, socketpool_socket_settimeout);

//|     type: int
//|     """Read-only access to the socket type"""
//|
static mp_obj_t socketpool_socket_obj_get_type(mp_obj_t self_in) {
    socketpool_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_NEW_SMALL_INT(common_hal_socketpool_socket_get_type(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(socketpool_socket_get_type_obj, socketpool_socket_obj_get_type);

MP_PROPERTY_GETTER(socketpool_socket_type_obj,
    (mp_obj_t)&socketpool_socket_get_type_obj);

static const mp_rom_map_elem_t socketpool_socket_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&default___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&socketpool_socket___exit___obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&socketpool_socket_close_obj) },


    { MP_ROM_QSTR(MP_QSTR_accept), MP_ROM_PTR(&socketpool_socket_accept_obj) },
    { MP_ROM_QSTR(MP_QSTR_bind), MP_ROM_PTR(&socketpool_socket_bind_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&socketpool_socket_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&socketpool_socket_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_listen), MP_ROM_PTR(&socketpool_socket_listen_obj) },
    { MP_ROM_QSTR(MP_QSTR_recvfrom_into), MP_ROM_PTR(&socketpool_socket_recvfrom_into_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv_into), MP_ROM_PTR(&socketpool_socket_recv_into_obj) },
    { MP_ROM_QSTR(MP_QSTR_sendall), MP_ROM_PTR(&socketpool_socket_sendall_obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&socketpool_socket_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_sendto), MP_ROM_PTR(&socketpool_socket_sendto_obj) },
    { MP_ROM_QSTR(MP_QSTR_setblocking), MP_ROM_PTR(&socketpool_socket_setblocking_obj) },
    { MP_ROM_QSTR(MP_QSTR_setsockopt), MP_ROM_PTR(&socketpool_socket_setsockopt_obj) },
    { MP_ROM_QSTR(MP_QSTR_settimeout), MP_ROM_PTR(&socketpool_socket_settimeout_obj) },
    { MP_ROM_QSTR(MP_QSTR_type), MP_ROM_PTR(&socketpool_socket_type_obj) },
};

static MP_DEFINE_CONST_DICT(socketpool_socket_locals_dict, socketpool_socket_locals_dict_table);

static mp_uint_t socket_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errorcode) {
    socketpool_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t ret = socketpool_socket_recv_into(self, buf, size);
    if (ret < 0) {
        *errorcode = -ret;
        return MP_STREAM_ERROR;
    }
    return ret;
}

static mp_uint_t socket_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errorcode) {
    socketpool_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t ret = socketpool_socket_send(self, buf, size);
    if (ret < 0) {
        *errorcode = -ret;
        return MP_STREAM_ERROR;
    }
    return ret;
}

static mp_uint_t socket_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode) {
    socketpool_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_uint_t ret;
    if (request == MP_STREAM_POLL) {
        mp_uint_t flags = arg;
        ret = 0;
        if ((flags & MP_STREAM_POLL_RD) && common_hal_socketpool_readable(self) > 0) {
            ret |= MP_STREAM_POLL_RD;
        }
        if ((flags & MP_STREAM_POLL_WR) && common_hal_socketpool_writable(self)) {
            ret |= MP_STREAM_POLL_WR;
        }
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}

static const mp_stream_p_t socket_stream_p = {
    .read = socket_read,
    .write = socket_write,
    .ioctl = socket_ioctl,
    .is_text = false,
};

MP_DEFINE_CONST_OBJ_TYPE(
    socketpool_socket_type,
    MP_QSTR_Socket,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    locals_dict, &socketpool_socket_locals_dict,
    protocol, &socket_stream_p
    );
