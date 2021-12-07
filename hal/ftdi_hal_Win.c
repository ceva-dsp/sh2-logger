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

#include "ftdi_hal.h"

#include <Windows.h>
#include <stdbool.h>
#include <stdio.h>
#include "ftd2xx.h"

#include "sh2_err.h"

#define RFC1662_FLAG (0x7E)
#define RFC1662_ESCAPE (0x7D)
#define PROTOCOL_CONTROL (0)
#define PROTOCOL_SHTP (1)

#define DEFAULT_BAUD_RATE (3000000)

static const UCHAR LATENCY_TIMER = 1;
static const UCHAR LATENCY_TIMER_STARTUP = 10;
static const uint32_t RESET_DELAY_US = 10000;
static const uint32_t DFU_BOOT_DELAY_US = 50000;
static const uint32_t SH2_BOOT_DELAY_US = 150000;

typedef enum {
    OUTSIDE_FRAME, // Waiting for start of frame
    INSIDE_FRAME,  // Inside frame until end of frame
    ESCAPED,       // Inside frame, after escape char
} RxState_t;

// Augmented HAL structure with BNO DFU on Windows specific fields.
struct ftdi_hal_s {
    sh2_Hal_t hal_fns;            // must be first so (sh2_Hal_t *) can be cast as (ftdi_hal_t *)

    bool dfu;
    DWORD baud;
    bool is_open;
    bool latencySet;
    unsigned deviceIdx;
    HANDLE ftHandle;
    HANDLE commEvent;
    uint16_t lastBsn;
    uint8_t rxFrame[SH2_HAL_MAX_PAYLOAD_IN];
    uint32_t rxFrameLen;
    bool rxFrameReady;
    uint32_t rxFrameStartTime_us;
    RxState_t rxState;
    bool anyRx;
    // TODO-DW : make sure all fields initialized in init(), open()
};
typedef struct ftdi_hal_s ftdi_hal_t;

// Set RESETN to state.
static void setResetN(ftdi_hal_t* pHal, bool state) {
    if (state) {
        FT_ClrDtr(pHal->ftHandle);
    } else {
        FT_SetDtr(pHal->ftHandle);
    }
}

// Set BOOTN to state.
static void setBootN(ftdi_hal_t* pHal, bool state) {
    if (state) {
        FT_ClrRts(pHal->ftHandle);
    } else {
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

static void rfc1662_reset(ftdi_hal_t* pHal) {
    pHal->rxFrameLen = 0;
    pHal->rxFrameReady = false;
    pHal->rxState = OUTSIDE_FRAME;
}

static void rfc1662_decode(ftdi_hal_t* pHal, uint8_t c) {
    // Use state machine to build up chars into frames for delivery.
    switch (pHal->rxState) {
        case OUTSIDE_FRAME:
            // Look for start of frame
            if (c == RFC1662_FLAG) {
                // Init frame in progress
                pHal->rxFrameStartTime_us = time32_now_us();
                pHal->rxFrameLen = 0;
                pHal->rxFrameReady = false;
                pHal->rxState = INSIDE_FRAME;
            }
            break;
        case INSIDE_FRAME:
            // Look for end of frame
            if (c == RFC1662_FLAG) {
                if (pHal->rxFrameLen > 0) {
                    // Frame is done
                    pHal->rxFrameReady = true;
                    pHal->rxState = OUTSIDE_FRAME;
                } else {
                    // Treat second consec flag as another start flag.
                    pHal->rxState = INSIDE_FRAME;
                }
            } else if (c == RFC1662_ESCAPE) {
                // Go to escaped state so next char can be a flag or escape
                pHal->rxState = ESCAPED;
            } else {
                if (pHal->rxFrameLen < sizeof(pHal->rxFrame)) {
                    pHal->rxFrame[pHal->rxFrameLen] = c;
                    pHal->rxFrameLen++;
                } else {
                    // overflowing, don't store data beyond allocated buffer size.
                    pHal->rxFrameLen++;
                }
            }
            break;
        case ESCAPED:
            if (pHal->rxFrameLen < sizeof(pHal->rxFrame)) {
                pHal->rxFrame[pHal->rxFrameLen] = c ^ 0x20;
                pHal->rxFrameLen++;
            } else {
                // overflowing, don't store data beyond allocated buffer size.
                pHal->rxFrameLen++;
            }
            pHal->rxState = INSIDE_FRAME;
            break;
        default:
            // Bad state.  Recover by resetting to outside frame state
            pHal->rxState = OUTSIDE_FRAME;
            break;
    }
}

static int ftdi_hal_open(sh2_Hal_t *self) {
    ftdi_hal_t *pHal = (ftdi_hal_t *)self;

    // Return error if already open
    if (pHal->is_open) return SH2_ERR;

    // Mark as open
    pHal->is_open = true;

    // clear anyRx flag.  This will be used to establish LATENCY_TIMER once data is received.
    pHal->anyRx = false;

    // reset de-framer
    rfc1662_reset(pHal);

    pHal->lastBsn = 0;
    pHal->rxFrameStartTime_us = time32_now_us();
                        
    FT_STATUS status;
    status = FT_Open(pHal->deviceIdx, &pHal->ftHandle);
    if (status != FT_OK) {
        fprintf(stderr, "Unable to find an FTDI COM port\n");
        return SH2_ERR_BAD_PARAM;
    }

    LONG comPort = -1;
    status = FT_GetComPortNumber(pHal->ftHandle, &comPort);
    fprintf(stderr, "FTDI device found on COM%d\n", comPort);

    status = FT_SetBaudRate(pHal->ftHandle, pHal->baud);
    if (status != FT_OK) {
        fprintf(stderr, "Unable to set baud rate to: %d\n", pHal->baud);
        return SH2_ERR_BAD_PARAM;
    }

    status = FT_SetFlowControl(pHal->ftHandle, FT_FLOW_NONE, 0, 0);
    if (status != FT_OK) {
        fprintf(stderr, "Failed to set flow control\n");
    }

    status = FT_SetDataCharacteristics(pHal->ftHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
    if (status != FT_OK) {
        fprintf(stderr, "Unable to set data characteristics\n");
        return SH2_ERR_IO;
    }

    status = FT_SetLatencyTimer(pHal->ftHandle, LATENCY_TIMER_STARTUP);
    if (status != FT_OK) {
        fprintf(stderr, "Unable to set latency timer to: %d\n", LATENCY_TIMER_STARTUP);
        return SH2_ERR_IO;
    }

    status = FT_SetTimeouts(pHal->ftHandle, 1000, 3000);
    if (status != FT_OK) {
        fprintf(stderr, "Unable to set timeouts.\n");
        return SH2_ERR_IO;
    }

    pHal->commEvent = CreateEvent(NULL, false, false, "");
    FT_SetEventNotification(pHal->ftHandle, FT_EVENT_RXCHAR, pHal->commEvent);

    // Reset into bootloader
    setResetN(pHal, false);   // Assert reset
    if (pHal->dfu) {
        setBootN(pHal, false);    // Assert BOOTN
    }
    else {
        setBootN(pHal, true);    // Assert BOOTN
    }
    
    // Delay for RESET_DELAY_US to ensure reset takes effect
    delay_us(RESET_DELAY_US);
    
    setResetN(pHal, true);    // Deassert reset with BOOTN asserted

    // Wait until we know system is up
    if (pHal->dfu) {
        delay_us(DFU_BOOT_DELAY_US);
    }
    else {
        delay_us(SH2_BOOT_DELAY_US);
    }
    
    return SH2_OK;
}

static void ftdi_hal_close(sh2_Hal_t *self) {
    ftdi_hal_t* pHal = (ftdi_hal_t*)self;

    // Reset into normal SHTP mode
    setResetN(pHal, false); // Assert reset
    setBootN(pHal, true);   // De-assert BOOTN

    // Delay for RESET_DELAY_US to ensure reset takes effect
    delay_us(RESET_DELAY_US);

    // Mark as not open
    pHal->is_open = false;

    // Close descriptor
    FT_Close(pHal->ftHandle);
}

static int ftdi_hal_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us) {
    int retval = 0;
    DWORD rxBytes;
    DWORD txBytes;
    DWORD eventDWord;
    uint8_t c;
    DWORD bytesRead;

    ftdi_hal_t* pHal = (ftdi_hal_t*)self;
    
    // Figure out how much we can read
	FT_GetStatus(pHal->ftHandle, &rxBytes, &txBytes, &eventDWord);

    // process as many input chars as we can
    for (unsigned n = 0; n < rxBytes; n++) {
        if (FT_Read(pHal->ftHandle, &c, 1, &bytesRead) == FT_OK) {

            // Reset LATENCY_TIMER as soon as data is received.
            if (!pHal->anyRx) {
                FT_SetLatencyTimer(pHal->ftHandle, LATENCY_TIMER);
                pHal->anyRx = true;
            }

            // Decode this char
            rfc1662_decode(pHal, c);

            // If a full frame is ready
            if (pHal->rxFrameReady) {
                if ((pHal->rxFrameLen > 0) && (pHal->rxFrame[0] == PROTOCOL_CONTROL)) {
                    // This is a BSN
                    pHal->lastBsn = (pHal->rxFrame[2]<<8) + pHal->rxFrame[1];
                    rfc1662_reset(pHal);
                }
                else {
                    // An SHTP frame is in rxFrame
                    if (pHal->rxFrameLen > sizeof(pHal->rxFrame)) {
                        // frame was truncated, discard it.
                        rfc1662_reset(pHal);
                    }
                    else if (pHal->rxFrameLen > len) {
                        // frame won't fit in user's buffer, discard it.
                        rfc1662_reset(pHal);
                    }
                    else {
                        // Copy the frame into user's buffer
                        // (Don't include first byte, the UART protocol id field.)
                        memcpy(pBuffer, pHal->rxFrame+1, pHal->rxFrameLen-1);
                        retval = pHal->rxFrameLen;
                        *t_us = pHal->rxFrameStartTime_us;
                        rfc1662_reset(pHal);
                        break;
                    }
                }
                pHal->rxFrameReady = false;
            }
        }
        else {
            // Error from read, abort this frame
            rfc1662_reset(pHal);
        }
    }

    return retval;
}

static bool txEncode(uint8_t* pOut, uint32_t* outLen, uint8_t* pIn, uint32_t inLen) {
    bool overflowed = false;
    uint32_t outIndex = 0;

    // start of frame
    if (outIndex >= *outLen) return true;  // overflowed
    pOut[outIndex++] = RFC1662_FLAG;

    // protocol id
    if (outIndex >= *outLen) return true; // overflowed
    pOut[outIndex++] = PROTOCOL_SHTP;
    
    // RFC1662 encoded data
    for (unsigned i = 0; i < inLen; i++) {
        if ((pIn[i] == RFC1662_FLAG) || (pIn[i] == RFC1662_ESCAPE)) {
            // escape this char
            if (outIndex + 1 >= *outLen)
                return true; // overflowed
            pOut[outIndex++] = RFC1662_ESCAPE;
            pOut[outIndex++] = pIn[i] ^ 0x20;
        } else {
            if (outIndex >= *outLen)
                return true;
            pOut[outIndex++] = pIn[i];
        }
    }

    // end of frame
    if (outIndex >= *outLen)
        return true;  // overflowed
    pOut[outIndex++] = RFC1662_FLAG;

    // set outLen for return
    *outLen = outIndex;
    return false;   // return, no overflow
}


static const uint8_t BSQ[] = {
    RFC1662_FLAG, PROTOCOL_CONTROL, RFC1662_FLAG
};


static int ftdi_hal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len) {
    int retval = 0;

    ftdi_hal_t* pHal = (ftdi_hal_t*)self;
       
    // encode pBuffer
    uint8_t writeBuf[1024];  // TODO-DW : review size.
    uint32_t encodedLen = sizeof(writeBuf);
    bool overflow = txEncode(writeBuf, &encodedLen, pBuffer, len);
    if (overflow) {
        return SH2_ERR_BAD_PARAM;  
    }
    
    // if device is ready to receive this size transfer...
    if (pHal->lastBsn >= encodedLen) {
        // write bytes one at a time.
        // The sensor hub can't handle data too fast, we need to limit it to
        // one char per millisecond, approximately.  By writing one char at a time
        // and having the USB latency setting at 1ms, we can effectively limit
        // the data rate as needed.
        unsigned written = 0;
        for (unsigned n = 0; n < encodedLen; n+=written) {
            // transmit c.
            FT_STATUS status = FT_Write(pHal->ftHandle, writeBuf+n, 1, &written);
            if (status != FT_OK) {
                // fail with I/O error
                return SH2_ERR_IO;
            }
        }
        
        // set retval to notify caller that data was sent
        retval = len;
    }
    else {
        // transmit a buffer status query to ensure we get a buffer
        // status notification update.
        int written = 0;
        for (unsigned n = 0; n < sizeof(BSQ); n += written) {
            FT_STATUS status = FT_Write(pHal->ftHandle, (void *)(BSQ + n), 1, &written);
            if (status != FT_OK) {
                // fail with I/O error
                return SH2_ERR_IO;
            }
        }

        // we did not write any of the user's data
        retval = 0;
    }
        
    return retval;
}

static int ftdi_hal_getTimeUs(sh2_Hal_t *self) {
    return time32_now_us();
}

// regular (non-dfu) hal instance data
static ftdi_hal_t ftdi_hal = {
    .hal_fns = {
        .open = ftdi_hal_open,
        .close = ftdi_hal_close,
        .read = ftdi_hal_read,
        .write = ftdi_hal_write,
        .getTimeUs = ftdi_hal_getTimeUs,
    },

    .dfu = false,
    .baud = DEFAULT_BAUD_RATE,
    .is_open = false,
    .latencySet = false,
    .deviceIdx = 0,
    .ftHandle = 0,
    .commEvent = 0,
};

// dfu_hal instance data
static ftdi_hal_t dfu_hal = {
    .hal_fns = {
        .open = ftdi_hal_open,
        .close = ftdi_hal_close,
        .read = ftdi_hal_read,
        .write = ftdi_hal_write,
        .getTimeUs = ftdi_hal_getTimeUs,
    },

    .dfu = true,
    .baud = DEFAULT_BAUD_RATE,
    .is_open = false,
    .latencySet = false,
    .deviceIdx = 0,
    .ftHandle = 0,
    .commEvent = 0,
};

// ---------------------------------------------------------
// Public functions

sh2_Hal_t * ftdi_hal_init(unsigned deviceIdx) {
    // Save reference to device file name, etc.
    ftdi_hal.deviceIdx = deviceIdx;
    ftdi_hal.dfu = false;
    ftdi_hal.baud = DEFAULT_BAUD_RATE;

    // give caller the list of access functions.
    return &(ftdi_hal.hal_fns);
}

sh2_Hal_t * ftdi_hal_autobaud_init(unsigned deviceIdx, uint32_t baud) {
    // Save reference to device file name, etc.
    ftdi_hal.deviceIdx = deviceIdx;
    ftdi_hal.dfu = false;
    ftdi_hal.baud = baud;

    // give caller the list of access functions.
    return &(ftdi_hal.hal_fns);
}

sh2_Hal_t * ftdi_hal_dfu_init(unsigned deviceIdx) {
    // Save reference to device file name, etc.
    dfu_hal.deviceIdx = deviceIdx;
    ftdi_hal.dfu = true;
    ftdi_hal.baud = DEFAULT_BAUD_RATE;

    // give caller the list of access functions.
    return &(dfu_hal.hal_fns);
}

sh2_Hal_t * ftdi_hal_autobaud_dfu_init(unsigned deviceIdx, uint32_t baud) {
    // Save reference to device file name, etc.
    dfu_hal.deviceIdx = deviceIdx;
    ftdi_hal.dfu = true;
    ftdi_hal.baud = baud;

    // give caller the list of access functions.
    return &(dfu_hal.hal_fns);
}

