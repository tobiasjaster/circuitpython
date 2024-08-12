// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2016 Scott Shawcroft
// SPDX-FileCopyrightText: Copyright (c) 2019 Artur Pacholec
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/busio/SPI.h"
#include "py/mperrno.h"
#include "py/runtime.h"
#include "periph.h"

#include "sdk/drivers/lpspi/fsl_lpspi.h"

#include <stdio.h>

#if IMXRT11XX
#define LPSPI_MASTER_CLK_FREQ (24000000)
#else
#define LPSPI_MASTER_CLK_FREQ (CLOCK_GetFreq(kCLOCK_Usb1PllPfd0Clk) / (CLOCK_GetDiv(kCLOCK_LpspiDiv) + 1))
#endif

#define MAX_SPI_BUSY_RETRIES 100

// arrays use 0 based numbering: SPI1 is stored at index 0
static bool reserved_spi[MP_ARRAY_SIZE(mcu_spi_banks)];
static bool never_reset_spi[MP_ARRAY_SIZE(mcu_spi_banks)];

#if IMXRT11XX
static const clock_ip_name_t s_lpspiClocks[] = LPSPI_CLOCKS;
#endif

static void config_periph_pin(const mcu_periph_obj_t *periph) {
    IOMUXC_SetPinMux(
        periph->pin->mux_reg, periph->mux_mode,
        periph->input_reg, periph->input_idx,
        0,
        0);

    IOMUXC_SetPinConfig(0, 0, 0, 0,
        periph->pin->cfg_reg,
        IOMUXC_SW_PAD_CTL_PAD_PUS(0)
        #if IMXRT10XX
        | IOMUXC_SW_PAD_CTL_PAD_HYS(0)
        | IOMUXC_SW_PAD_CTL_PAD_PKE(1)
        | IOMUXC_SW_PAD_CTL_PAD_SPEED(2)
        #endif
        | IOMUXC_SW_PAD_CTL_PAD_PUE(0)
        | IOMUXC_SW_PAD_CTL_PAD_ODE(0)
        | IOMUXC_SW_PAD_CTL_PAD_DSE(4)
        | IOMUXC_SW_PAD_CTL_PAD_SRE(0));
}

void spi_reset(void) {
    for (uint i = 0; i < MP_ARRAY_SIZE(mcu_spi_banks); i++) {
        if (!never_reset_spi[i]) {
            reserved_spi[i] = false;
            #if IMXRT11XX
            // Skip resetting SPIs that aren't clocked. Doing so generates a bus fault.
            if ((CCM->LPCG[s_lpspiClocks[i + 1]].STATUS0 & CCM_LPCG_STATUS0_ON_MASK) == ((uint32_t)kCLOCK_Off & CCM_LPCG_STATUS0_ON_MASK)) {
                continue;
            }
            #endif

            LPSPI_Deinit(mcu_spi_banks[i]);
        }
    }
}

void common_hal_busio_spi_construct(busio_spi_obj_t *self,
    const mcu_pin_obj_t *clock, const mcu_pin_obj_t *mosi,
    const mcu_pin_obj_t *miso, bool half_duplex) {

    const uint32_t sck_count = MP_ARRAY_SIZE(mcu_spi_sck_list);
    const uint32_t miso_count = MP_ARRAY_SIZE(mcu_spi_sdi_list);
    const uint32_t mosi_count = MP_ARRAY_SIZE(mcu_spi_sdo_list);
    bool spi_taken = false;

    if (half_duplex) {
        mp_raise_NotImplementedError_varg(MP_ERROR_TEXT("%q"), MP_QSTR_half_duplex);
    }

    for (uint i = 0; i < sck_count; i++) {
        if (mcu_spi_sck_list[i].pin != clock) {
            continue;
        }
        // if both MOSI and MISO exist, loop search normally
        if ((mosi != NULL) && (miso != NULL)) {
            for (uint j = 0; j < mosi_count; j++) {
                if ((mcu_spi_sdo_list[j].pin != mosi)
                    || (mcu_spi_sck_list[i].bank_idx != mcu_spi_sdo_list[j].bank_idx)) {
                    continue;
                }
                for (uint k = 0; k < miso_count; k++) {
                    if ((mcu_spi_sdi_list[k].pin != miso) // everything needs the same index
                        || (mcu_spi_sck_list[i].bank_idx != mcu_spi_sdi_list[k].bank_idx)) {
                        continue;
                    }
                    // if SPI is taken, break (pins never have >1 periph)
                    if (reserved_spi[mcu_spi_sck_list[i].bank_idx - 1]) {
                        spi_taken = true;
                        break;
                    }
                    // store pins if not
                    self->clock = &mcu_spi_sck_list[i];
                    self->mosi = &mcu_spi_sdo_list[j];
                    self->miso = &mcu_spi_sdi_list[k];
                    break;
                }
                if (self->clock != NULL || spi_taken) {
                    break; // Multi-level break to pick lowest peripheral
                }
            }
            if (self->clock != NULL || spi_taken) {
                break;
            }
            // if just MISO, reduce search
        } else if (miso != NULL) {
            for (uint j = 0; j < miso_count; j++) {
                if ((mcu_spi_sdi_list[j].pin != miso) // only SCK and MISO need the same index
                    || (mcu_spi_sck_list[i].bank_idx != mcu_spi_sdi_list[j].bank_idx)) {
                    continue;
                }
                if (reserved_spi[mcu_spi_sck_list[i].bank_idx - 1]) {
                    spi_taken = true;
                    break;
                }
                self->clock = &mcu_spi_sck_list[i];
                self->miso = &mcu_spi_sdi_list[j];
                break;
            }
            if (self->clock != NULL || spi_taken) {
                break;
            }
            // if just MOSI, reduce search
        } else if (mosi != NULL) {
            for (uint j = 0; j < mosi_count; j++) {
                if ((mcu_spi_sdo_list[j].pin != mosi) // only SCK and MOSI need the same index
                    || (mcu_spi_sck_list[i].bank_idx != mcu_spi_sdo_list[j].bank_idx)) {
                    continue;
                }
                if (reserved_spi[mcu_spi_sck_list[i].bank_idx - 1]) {
                    spi_taken = true;
                    break;
                }
                self->clock = &mcu_spi_sck_list[i];
                self->mosi = &mcu_spi_sdo_list[j];
                break;
            }
            if (self->clock != NULL || spi_taken) {
                break;
            }
        } else {
            // throw an error immediately
            mp_raise_ValueError(MP_ERROR_TEXT("Must provide MISO or MOSI pin"));
        }
    }

    if (self->clock != NULL && (self->mosi != NULL || self->miso != NULL)) {
        self->spi = mcu_spi_banks[self->clock->bank_idx - 1];
    } else {
        if (spi_taken) {
            mp_raise_ValueError(MP_ERROR_TEXT("Hardware in use, try alternative pins"));
        } else {
            raise_ValueError_invalid_pins();
        }
    }

    config_periph_pin(self->clock);
    if (self->mosi != NULL) {
        config_periph_pin(self->mosi);
    }
    if (self->miso != NULL) {
        config_periph_pin(self->miso);
    }
    reserved_spi[self->clock->bank_idx - 1] = true;

    lpspi_master_config_t config = { 0 };
    LPSPI_MasterGetDefaultConfig(&config);

    // Always start at 250khz which is what SD cards need. They are sensitive to
    // SPI bus noise before they are put into SPI mode.
    config.baudRate = 250000;
    LPSPI_MasterInit(self->spi, &config, LPSPI_MASTER_CLK_FREQ);
    LPSPI_Enable(self->spi, false);
    uint32_t tcrPrescaleValue;
    self->baudrate = LPSPI_MasterSetBaudRate(self->spi, config.baudRate, LPSPI_MASTER_CLK_FREQ, &tcrPrescaleValue);
    self->spi->TCR = (self->spi->TCR & ~LPSPI_TCR_PRESCALE_MASK) | LPSPI_TCR_PRESCALE(tcrPrescaleValue);
    LPSPI_Enable(self->spi, true);

    claim_pin(self->clock->pin);
    if (self->mosi != NULL) {
        claim_pin(self->mosi->pin);
    }
    if (self->miso != NULL) {
        claim_pin(self->miso->pin);
    }
}

void common_hal_busio_spi_never_reset(busio_spi_obj_t *self) {
    never_reset_spi[self->clock->bank_idx - 1] = true;
    common_hal_never_reset_pin(self->clock->pin);
    if (self->mosi != NULL) {
        common_hal_never_reset_pin(self->mosi->pin);
    }
    if (self->miso != NULL) {
        common_hal_never_reset_pin(self->miso->pin);
    }
}

bool common_hal_busio_spi_deinited(busio_spi_obj_t *self) {
    return self->clock == NULL;
}

void common_hal_busio_spi_deinit(busio_spi_obj_t *self) {
    if (common_hal_busio_spi_deinited(self)) {
        return;
    }
    LPSPI_Deinit(self->spi);
    reserved_spi[self->clock->bank_idx - 1] = false;
    never_reset_spi[self->clock->bank_idx - 1] = false;

    common_hal_reset_pin(self->clock->pin);
    common_hal_reset_pin(self->mosi->pin);
    common_hal_reset_pin(self->miso->pin);

    self->clock = NULL;
    self->mosi = NULL;
    self->miso = NULL;
}

bool common_hal_busio_spi_configure(busio_spi_obj_t *self,
    uint32_t baudrate, uint8_t polarity, uint8_t phase, uint8_t bits) {

    if (baudrate > 30000000) {
        baudrate = 30000000; // "Absolute maximum frequency of operation (fop) is 30 MHz" -- IMXRT1010CEC.pdf
    }

    if ((polarity == common_hal_busio_spi_get_polarity(self)) &&
        (phase == common_hal_busio_spi_get_phase(self)) &&
        (bits == ((self->spi->TCR & LPSPI_TCR_FRAMESZ_MASK) >> LPSPI_TCR_FRAMESZ_SHIFT)) + 1 &&
        (baudrate == common_hal_busio_spi_get_frequency(self))) {
        return true;
    }

    lpspi_master_config_t config = { 0 };
    LPSPI_MasterGetDefaultConfig(&config);

    config.baudRate = baudrate;
    config.cpol = polarity;
    config.cpha = phase;
    config.bitsPerFrame = bits;
    // The between-transfer-delay must be equal to the SCK low-time.
    // Setting it lower introduces runt pulses, while setting it higher
    // wastes time.
    config.betweenTransferDelayInNanoSec = (1000000000 / config.baudRate) / 2;

    LPSPI_Deinit(self->spi);
    LPSPI_MasterInit(self->spi, &config, LPSPI_MASTER_CLK_FREQ);

    // Recompute the actual baudrate so that we can set the baudrate
    // (frequency) property.  We don't need to set TCR because it was
    // established by LPSPI_MasterInit, above
    uint32_t tcrPrescaleValue;
    LPSPI_Enable(self->spi, false);
    self->baudrate = LPSPI_MasterSetBaudRate(self->spi, baudrate, LPSPI_MASTER_CLK_FREQ, &tcrPrescaleValue);
    LPSPI_Enable(self->spi, true);

    return true;
}

bool common_hal_busio_spi_try_lock(busio_spi_obj_t *self) {
    bool grabbed_lock = false;
//    CRITICAL_SECTION_ENTER()
    if (!self->has_lock) {
        grabbed_lock = true;
        self->has_lock = true;
    }
//    CRITICAL_SECTION_LEAVE();
    return grabbed_lock;
}

bool common_hal_busio_spi_has_lock(busio_spi_obj_t *self) {
    return self->has_lock;
}

void common_hal_busio_spi_unlock(busio_spi_obj_t *self) {
    self->has_lock = false;
}

static status_t transfer_common(busio_spi_obj_t *self, lpspi_transfer_t *xfer) {
    xfer->configFlags = kLPSPI_MasterPcsContinuous;

    status_t status;
    int retries = MAX_SPI_BUSY_RETRIES;
    do {
        status = LPSPI_MasterTransferBlocking(self->spi, xfer);
    } while (status == kStatus_LPSPI_Busy && --retries > 0);

    if (status != kStatus_Success) {
        printf("%s: status %ld\r\n", __func__, status);
    }
    return status;
}

bool common_hal_busio_spi_write(busio_spi_obj_t *self,
    const uint8_t *data, size_t len) {
    if (len == 0) {
        return true;
    }
    if (self->mosi == NULL) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("No %q pin"), MP_QSTR_mosi);
    }

    lpspi_transfer_t xfer = { 0 };
    xfer.txData = (uint8_t *)data;
    xfer.dataSize = len;

    status_t status = transfer_common(self, &xfer);

    return status == kStatus_Success;
}

bool common_hal_busio_spi_read(busio_spi_obj_t *self,
    uint8_t *data, size_t len, uint8_t write_value) {
    if (len == 0) {
        return true;
    }
    if (self->miso == NULL) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("No %q pin"), MP_QSTR_miso);
    }

    LPSPI_SetDummyData(self->spi, write_value);

    lpspi_transfer_t xfer = { 0 };
    xfer.rxData = data;
    xfer.dataSize = len;

    status_t status = transfer_common(self, &xfer);

    return status == kStatus_Success;
}

bool common_hal_busio_spi_transfer(busio_spi_obj_t *self, const uint8_t *data_out, uint8_t *data_in, size_t len) {
    if (len == 0) {
        return true;
    }
    if (self->mosi == NULL && data_out != NULL) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("No %q pin"), MP_QSTR_mosi);
    }
    if (self->miso == NULL && data_in != NULL) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("No %q pin"), MP_QSTR_miso);
    }

    LPSPI_SetDummyData(self->spi, 0xFF);

    lpspi_transfer_t xfer = { 0 };
    xfer.txData = (uint8_t *)data_out;
    xfer.rxData = data_in;
    xfer.dataSize = len;

    status_t status = transfer_common(self, &xfer);

    return status == kStatus_Success;
}

uint32_t common_hal_busio_spi_get_frequency(busio_spi_obj_t *self) {
    return self->baudrate;
}

uint8_t common_hal_busio_spi_get_phase(busio_spi_obj_t *self) {
    return (self->spi->TCR & LPSPI_TCR_CPHA_MASK) == LPSPI_TCR_CPHA_MASK;
}

uint8_t common_hal_busio_spi_get_polarity(busio_spi_obj_t *self) {
    return (self->spi->TCR & LPSPI_TCR_CPOL_MASK) == LPSPI_TCR_CPOL_MASK;
}
