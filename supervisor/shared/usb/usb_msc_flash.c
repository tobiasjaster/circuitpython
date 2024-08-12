// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2018 hathach for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "tusb.h"
// // #include "supervisor/flash.h"

// For updating fatfs's cache
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"
#include "lib/oofatfs/diskio.h"
#include "lib/oofatfs/ff.h"
#include "py/mpstate.h"

#include "shared-module/storage/__init__.h"
#include "supervisor/filesystem.h"
#include "supervisor/shared/reload.h"

#define MSC_FLASH_BLOCK_SIZE    512

static bool ejected[1] = {true};
static bool locked[1] = {false};

// The root FS is always at the end of the list.
static fs_user_mount_t *get_vfs(int lun) {
    // TODO(tannewt): Return the mount which matches the lun where 0 is the end
    // and is counted in reverse.
    if (lun > 0) {
        return NULL;
    }
    mp_vfs_mount_t *current_mount = MP_STATE_VM(vfs_mount_table);
    if (current_mount == NULL) {
        return NULL;
    }
    while (current_mount->next != NULL) {
        current_mount = current_mount->next;
    }
    return current_mount->obj;
}

static void _usb_msc_uneject(void) {
    for (uint8_t i = 0; i < sizeof(ejected); i++) {
        ejected[i] = false;
        locked[i] = false;
    }
}

void usb_msc_mount(void) {
    _usb_msc_uneject();
}

void usb_msc_umount(void) {
    for (uint8_t i = 0; i < sizeof(ejected); i++) {
        fs_user_mount_t *vfs = get_vfs(i + 1);
        if (vfs == NULL) {
            continue;
        }
        blockdev_unlock(vfs);
        locked[i] = false;
    }
}

bool usb_msc_ejected(void) {
    bool all_ejected = true;
    for (uint8_t i = 0; i < sizeof(ejected); i++) {
        all_ejected &= ejected[i];
    }
    return all_ejected;
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, TEST_UNIT_READY, START_STOP_UNIT, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 have their own callbacks
int32_t tud_msc_scsi_cb(uint8_t lun, const uint8_t scsi_cmd[16], void *buffer, uint16_t bufsize) {
    const void *response = NULL;
    int32_t resplen = 0;

    switch (scsi_cmd[0]) {
        case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
            // Host is about to read/write etc ... better not to disconnect disk
            resplen = 0;
            break;

        default:
            // Set Sense = Invalid Command Operation
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);

            // negative means error -> tinyusb could stall and/or response with failed status
            resplen = -1;
            break;
    }

    // return len must not larger than bufsize
    if (resplen > bufsize) {
        resplen = bufsize;
    }

    // copy response to stack's buffer if any
    if (response && (resplen > 0)) {
        memcpy(buffer, response, resplen);
    }

    return resplen;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size) {
    fs_user_mount_t *vfs = get_vfs(lun);
    disk_ioctl(vfs, GET_SECTOR_COUNT, block_count);
    disk_ioctl(vfs, GET_SECTOR_SIZE, block_size);
}

bool tud_msc_is_writable_cb(uint8_t lun) {
    if (lun > 1) {
        return false;
    }

    fs_user_mount_t *vfs = get_vfs(lun);
    if (vfs == NULL) {
        return false;
    }
    if (vfs->blockdev.writeblocks[0] == MP_OBJ_NULL || !filesystem_is_writable_by_usb(vfs)) {
        return false;
    }
    // Lock the blockdev once we say we're writable.
    if (!locked[lun] && !blockdev_lock(vfs)) {
        return false;
    }
    locked[lun] = true;
    return true;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
    (void)offset;

    const uint32_t block_count = bufsize / MSC_FLASH_BLOCK_SIZE;

    fs_user_mount_t *vfs = get_vfs(lun);
    uint32_t disk_block_count;
    disk_ioctl(vfs, GET_SECTOR_COUNT, &disk_block_count);

    if (lba + block_count > disk_block_count) {
        return -1;
    }

    disk_read(vfs, buffer, lba, block_count);

    return block_count * MSC_FLASH_BLOCK_SIZE;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
    (void)lun;
    (void)offset;
    autoreload_suspend(AUTORELOAD_SUSPEND_USB);

    const uint32_t block_count = bufsize / MSC_FLASH_BLOCK_SIZE;

    fs_user_mount_t *vfs = get_vfs(lun);
    disk_write(vfs, buffer, lba, block_count);
    // Since by getting here we assume the mount is read-only to
    // MicroPython let's update the cached FatFs sector if it's the one
    // we just wrote.
    #if FF_MAX_SS != FF_MIN_SS
    if (vfs->fatfs.ssize == MSC_FLASH_BLOCK_SIZE) {
    #else
    // The compiler can optimize this away.
    if (FF_MAX_SS == FILESYSTEM_BLOCK_SIZE) {
        #endif
        if (lba == vfs->fatfs.winsect && lba > 0) {
            memcpy(vfs->fatfs.win,
                buffer + MSC_FLASH_BLOCK_SIZE * (vfs->fatfs.winsect - lba),
                MSC_FLASH_BLOCK_SIZE);
        }
    }

    return block_count * MSC_FLASH_BLOCK_SIZE;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
void tud_msc_write10_complete_cb(uint8_t lun) {
    (void)lun;

    // This write is complete; initiate an autoreload.
    autoreload_resume(AUTORELOAD_SUSPEND_USB);
    autoreload_trigger();
}

// Invoked when received SCSI_CMD_INQUIRY
// Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
    (void)lun;

    memcpy(vendor_id, CFG_TUD_MSC_VENDOR, strlen(CFG_TUD_MSC_VENDOR));
    memcpy(product_id, CFG_TUD_MSC_PRODUCT, strlen(CFG_TUD_MSC_PRODUCT));
    memcpy(product_rev, CFG_TUD_MSC_PRODUCT_REV, strlen(CFG_TUD_MSC_PRODUCT_REV));
}

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    if (lun > 1) {
        return false;
    }

    fs_user_mount_t *current_mount = get_vfs(lun);
    if (current_mount == NULL) {
        return false;
    }
    if (ejected[lun]) {
        // Set 0x3a for media not present.
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3A, 0x00);
        return false;
    }

    return true;
}

// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    if (lun > 1) {
        return false;
    }
    fs_user_mount_t *current_mount = get_vfs(lun);
    if (current_mount == NULL) {
        return false;
    }
    if (load_eject) {
        if (!start) {
            // Eject but first flush.
            if (disk_ioctl(current_mount, CTRL_SYNC, NULL) != RES_OK) {
                return false;
            } else {
                blockdev_unlock(current_mount);
                ejected[lun] = true;
                locked[lun] = false;
            }
        } else {
            // We can only load if it hasn't been ejected.
            return !ejected[lun];
        }
    } else {
        if (!start) {
            // Stop the unit but don't eject.
            if (disk_ioctl(current_mount, CTRL_SYNC, NULL) != RES_OK) {
                return false;
            }
        }
        // Always start the unit, even if ejected. Whether media is present is a separate check.
    }

    return true;
}
