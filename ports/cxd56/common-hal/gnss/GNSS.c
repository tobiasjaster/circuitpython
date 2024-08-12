// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright 2020 Sony Semiconductor Solutions Corporation
//
// SPDX-License-Identifier: MIT

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#include "py/runtime.h"

#include "shared-bindings/gnss/GNSS.h"

typedef struct {
    const char *devpath;
    int fd;
} gnss_dev_t;

static gnss_dev_t gnss_dev = {"/dev/gps", -1};

static gnss_positionfix_t fix_to_positionfix_type(uint8_t fix) {
    switch (fix) {
        case CXD56_GNSS_PVT_POSFIX_2D:
            return POSITIONFIX_2D;
        case CXD56_GNSS_PVT_POSFIX_3D:
            return POSITIONFIX_3D;
        case CXD56_GNSS_PVT_POSFIX_INVALID:
        default:
            return POSITIONFIX_INVALID;
    }
}

void common_hal_gnss_construct(gnss_obj_t *self, unsigned long selection) {
    if (gnss_dev.fd < 0) {
        gnss_dev.fd = open(gnss_dev.devpath, O_RDONLY);
        if (gnss_dev.fd < 0) {
            mp_raise_RuntimeError(MP_ERROR_TEXT("GNSS init"));
        }
    }

    self->satellite_system = 0;
    self->fix = POSITIONFIX_INVALID;

    unsigned long sel = 0;

    if (selection & SATELLITESYSTEM_GPS) {
        sel |= CXD56_GNSS_SAT_GPS;
    } else if (selection & SATELLITESYSTEM_GLONASS) {
        sel |= CXD56_GNSS_SAT_GLONASS;
    } else if (selection & SATELLITESYSTEM_SBAS) {
        sel |= CXD56_GNSS_SAT_SBAS;
    } else if (selection & SATELLITESYSTEM_QZSS_L1CA) {
        sel |= CXD56_GNSS_SAT_QZ_L1CA;
    } else if (selection & SATELLITESYSTEM_QZSS_L1S) {
        sel |= CXD56_GNSS_SAT_QZ_L1S;
    }

    ioctl(gnss_dev.fd, CXD56_GNSS_IOCTL_SELECT_SATELLITE_SYSTEM, sel);
    ioctl(gnss_dev.fd, CXD56_GNSS_IOCTL_START, CXD56_GNSS_STMOD_COLD);
}

void common_hal_gnss_deinit(gnss_obj_t *self) {
    if (common_hal_gnss_deinited(self)) {
        return;
    }

    close(gnss_dev.fd);
    gnss_dev.fd = -1;
}

bool common_hal_gnss_deinited(gnss_obj_t *self) {
    return gnss_dev.fd < 0;
}

void common_hal_gnss_update(gnss_obj_t *self) {
    struct cxd56_gnss_positiondata_s positiondata;

    read(gnss_dev.fd, &positiondata, sizeof(struct cxd56_gnss_positiondata_s));

    if (positiondata.receiver.pos_dataexist) {
        self->fix = positiondata.receiver.pos_fixmode;
        self->latitude = positiondata.receiver.latitude;
        self->longitude = positiondata.receiver.longitude;
        self->altitude = positiondata.receiver.altitude;
        memcpy(&self->date, &positiondata.receiver.date, sizeof(struct cxd56_gnss_date_s));
        memcpy(&self->time, &positiondata.receiver.time, sizeof(struct cxd56_gnss_time_s));
    }
}

mp_float_t common_hal_gnss_get_latitude(gnss_obj_t *self) {
    return (mp_float_t)self->latitude;
}

mp_float_t common_hal_gnss_get_longitude(gnss_obj_t *self) {
    return (mp_float_t)self->longitude;
}

mp_float_t common_hal_gnss_get_altitude(gnss_obj_t *self) {
    return (mp_float_t)self->altitude;
}

void common_hal_gnss_get_timestamp(gnss_obj_t *self, timeutils_struct_time_t *tm) {
    tm->tm_year = self->date.year;
    tm->tm_mon = self->date.month;
    tm->tm_mday = self->date.day;
    tm->tm_hour = self->time.hour;
    tm->tm_min = self->time.minute;
    tm->tm_sec = self->time.sec;
}

gnss_positionfix_t common_hal_gnss_get_fix(gnss_obj_t *self) {
    return fix_to_positionfix_type(self->fix);
}
