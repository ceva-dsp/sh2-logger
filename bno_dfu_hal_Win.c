/*
 * Copyright 2021 CEVA, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License and
 * any applicable agreements you may have with CEVA, Inc.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bno_dfu_hal.h"

#include <Windows.h>
#include <stdbool.h>
#include <stdio.h>
#include "ftd2xx.h"

#include "sh2_err.h"

#define RESET_DELAY_US (10000)

// Wait this long before assuming bootloader is ready
#define DFU_BOOT_DELAY_US (50000)

// Wait this long before assuming SH2 is ready
#define SH2_BOOT_DELAY_US (150000)

#define CBUS_DTR (0x01)
#define CBUS_RTS (0x02)
#define CBUS_OUTPUTS ((CBUS_DTR | CBUS_RTS) << 4)
#define CBUS_ASYNC_BIT_BANG (0x01)  // Asynchronous bit bang mode

// Augmented HAL structure with BNO DFU on Windows specific fields.
struct bno_dfu_hal_s {
    sh2_Hal_t hal_fns;            // must be first so (sh2_Hal_t *) can be cast as (bno_dfu_hal_t *)
    
    bool is_open;
    bool latencySet;
    unsigned deviceIdx;
    HANDLE ftHandle;
    HANDLE commEvent;
};
typedef struct bno_dfu_hal_s bno_dfu_hal_t;

// Set RESETN to state.
static void setResetN(bno_dfu_hal_t *pHal, bool state) {
    if (state) {
        FT_ClrDtr(pHal->ftHandle);
    }
    else {
        FT_SetDtr(pHal->ftHandle);
    }
}

// Set BOOTN to state.
static void setBootN(bno_dfu_hal_t *pHal, bool state) {
    if (state) {
        FT_ClrRts(pHal->ftHandle);
    }
    else {
        FT_SetRts(pHal->ftHandle);
    }
}    

static uint32_t time32_now_us() {
    static uint64_t initialTimeUs = 0;
    static uint64_t freq = 0;
    uint64_t counterTime;
    uint64_t t_us = 0;

    QueryPerformanceCounter((LARGE_INTEGER*)&counterTime);

    if (freq == 0) {
        QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    }
    
	t_us = (uint64_t)(counterTime * 1000000 / freq);

    if (initialTimeUs == 0) {
        initialTimeUs = t_us;
    }
    t_us -= initialTimeUs;

    return (uint32_t)t_us;
}

static void delay_us(uint32_t t) {
    uint32_t now = time32_now_us();
    uint32_t start = now;
    while ((int32_t)(now - start) <= t) {
        now = time32_now_us();
    }
}

static const DWORD BAUD_RATE = 115200;
static const UCHAR LATENCY_TIMER = 1;
static const UCHAR LATENCY_TIMER_STARTUP = 10;

static int dfu_hal_open(sh2_Hal_t *self) {
    bno_dfu_hal_t *pHal = (bno_dfu_hal_t *)self;

    // Return error if already open
    if (pHal->is_open) return SH2_ERR;

    // Mark as open
    pHal->is_open = true;

    FT_STATUS status;
    status = FT_Open(pHal->deviceIdx, &pHal->ftHandle);
    if (status != FT_OK) {
        fprintf(stderr, "Unable to find an FTDI COM port\n");
        return -1;
    }

    LONG comPort = -1;
    status = FT_GetComPortNumber(pHal->ftHandle, &comPort);
    fprintf(stderr, "FTDI device found on COM%d\n", comPort);

    status = FT_SetBaudRate(pHal->ftHandle, BAUD_RATE);
    if (status != FT_OK) {
        fprintf(stderr, "Unable to set baud rate to: %d\n", BAUD_RATE);
        return -1;
    }

    status = FT_SetFlowControl(pHal->ftHandle, FT_FLOW_NONE, 0, 0);
    if (status != FT_OK) {
        fprintf(stderr, "Failed to set flow control\n");
    }

    status = FT_SetDataCharacteristics(pHal->ftHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
    if (status != FT_OK) {
        fprintf(stderr, "Unable to set data characteristics\n");
        return -1;
    }

    // status = FT_ClrDtr(pHal->ftHandle);
    // status = FT_SetDtr(pHal->ftHandle);

    status = FT_SetLatencyTimer(pHal->ftHandle, LATENCY_TIMER_STARTUP);
    if (status != FT_OK) {
        fprintf(stderr, "Unable to set latency timer to: %d\n", LATENCY_TIMER_STARTUP);
        return -1;
    }

    status = FT_SetTimeouts(pHal->ftHandle, 1000, 3000);
    if (status != FT_OK) {
        fprintf(stderr, "Unable to set timeouts.\n");
        return -1;
    }

    pHal->commEvent = CreateEvent(NULL, false, false, "");
    FT_SetEventNotification(pHal->ftHandle, FT_EVENT_RXCHAR, pHal->commEvent);

    // Reset into bootloader
    setResetN(pHal, false);   // Assert reset
    setBootN(pHal, false);    // Assert BOOTN
    
    // Delay for RESET_DELAY_US to ensure reset takes effect
    delay_us(RESET_DELAY_US);
    
    setResetN(pHal, true);    // Deassert reset with BOOTN asserted

    // Wait until we know bootloader is up
    delay_us(DFU_BOOT_DELAY_US);
    
    return SH2_OK;
}

static void dfu_hal_close(sh2_Hal_t *self) {
    bno_dfu_hal_t *pHal = (bno_dfu_hal_t *)self;
    
    // Reset into normal SHTP mode
    setResetN(pHal, false);   // Assert reset
    setBootN(pHal, true);     // De-assert BOOTN
    
    // Delay for RESET_DELAY_US to ensure reset takes effect
    delay_us(RESET_DELAY_US);
    
    setResetN(pHal, true);    // De-assert reset with BOOTN de-asserted

    // Delay to ensure fully booted before allowing system to continue.
    delay_us(SH2_BOOT_DELAY_US);
    
    // Mark as not open
    pHal->is_open = false;
    
    // Close descriptor
    FT_Close(pHal->ftHandle);
}

static int dfu_hal_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us) {
    bno_dfu_hal_t *pHal = (bno_dfu_hal_t *)self;
    int status = 0;
    unsigned len_read = 0;
    uint8_t *pCursor = pBuffer;
	DWORD eventDWord;
	DWORD txBytes;
	DWORD rxBytes;

    FT_GetStatus(pHal->ftHandle, &rxBytes, &txBytes, &eventDWord);
    if (rxBytes > 0) {
        // Try to read
        int actually_read = 0;
        int to_read = min(len-len_read, rxBytes);
        if (FT_Read(pHal->ftHandle, pCursor, to_read, &actually_read) == FT_OK) {
            if (actually_read > 0) {
                len_read += actually_read;
                pCursor += actually_read;
            }
        }
        
        if (!pHal->latencySet) {
            FT_SetLatencyTimer(pHal->ftHandle, LATENCY_TIMER);
            pHal->latencySet = true;
        }
    }

    // Grab timestamp
    *t_us = time32_now_us();

    // Return num bytes read (might be zero)
    return len_read;
}

static int dfu_hal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len) {
    bno_dfu_hal_t *pHal = (bno_dfu_hal_t *)self;
    uint8_t *pNext = pBuffer;
    int wrote = 0;

    // Write data from pBuffer until the full length is written.
    while (wrote < len) {
        int written = 0;
        int status = FT_Write(pHal->ftHandle, pNext, len-wrote, &written);
        if (status == FT_OK) {
            wrote += written;
            pNext += written;
        }
        else {
            break;
        }
    }
    
    // Return num bytes written
    return wrote;
}

static uint32_t dfu_hal_getTimeUs(sh2_Hal_t *self) {
    return time32_now_us();
}

// dfu_hal instance data
static bno_dfu_hal_t dfu_hal = {
    .hal_fns = {
        .open = dfu_hal_open,
        .close = dfu_hal_close,
        .read = dfu_hal_read,
        .write = dfu_hal_write,
        .getTimeUs = dfu_hal_getTimeUs,
    },
    
    .is_open = false,
    .latencySet = false,
    .deviceIdx = 0,
    .ftHandle = 0,
    .commEvent = 0,
};

sh2_Hal_t * bno_dfu_hal_init(unsigned deviceIdx) {
    // Save reference to device file name, etc.
    dfu_hal.deviceIdx = deviceIdx;

    dfu_hal.is_open = false;
    dfu_hal.latencySet = false;
    dfu_hal.ftHandle = 0;
    dfu_hal.commEvent = 0;

    dfu_hal.hal_fns.open = dfu_hal_open;
    dfu_hal.hal_fns.close = dfu_hal_close;
    dfu_hal.hal_fns.read = dfu_hal_read;
    dfu_hal.hal_fns.write = dfu_hal_write;
    dfu_hal.hal_fns.getTimeUs = dfu_hal_getTimeUs;

    // give caller the list of access functions.
    return &(dfu_hal.hal_fns);
}
