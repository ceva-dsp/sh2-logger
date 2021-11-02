/*
 * Copyright 2015-21 CEVA, Inc.
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


#include <string.h>
#include <stdio.h>

#include "BnoDfu.h"
extern "C" {
    #include "firmware-bno.h"
    #include "sh2_hal.h"
    #include "sh2_err.h"
}

#define DFU_MAX_ATTEMPTS (5)
#define DFU_SEND_TIMEOUT_US (100000)
#define DELAY_POST_DFU_US (10000)  // 10ms pause after DFU process completes
#define ACK ('s')

// Constructor
BnoDfu::BnoDfu() {
    pHal = 0;
}

// I/O Utility functions
int BnoDfu::dfuSend(uint8_t* pData, uint32_t len)
{
    unsigned int retries = 0;
    int status = SH2_OK;
    uint8_t ack = 0;
    bool gotAck = false;
    uint32_t t;

    while (!gotAck && (retries < DFU_MAX_ATTEMPTS)) {
        uint32_t now = pHal->getTimeUs(pHal);
        uint32_t start = now;
        
        // Do write
        status = 0;
        while ((status == 0) && ((now - start) < DFU_SEND_TIMEOUT_US))
        {
            status = pHal->write(pHal, pData, len);
            now = pHal->getTimeUs(pHal);
        }
        if (status == 0)
        {
            // recognize timeout as an error.
            status = SH2_ERR_TIMEOUT;
        }

        // If write succeeded, read ack
        if (status > 0)
        {
            status = 0;
            while ((status == 0) && ((now - start) < DFU_SEND_TIMEOUT_US))
            {
                status = pHal->read(pHal, &ack, 1, &t);
                now = pHal->getTimeUs(pHal);
            }
            if (status == 0)
            {
                // Recognize timeout as an error
                status = SH2_ERR_TIMEOUT;
            }
        }

        
        // If read succeeded, check for ACK
        if (status > 0)
        {
            if (ack == ACK)
            {
                // We got a good ack
                gotAck = true;
                status = SH2_OK;
            }
            else
            {
                // We got NAK
                gotAck = false;
                status = SH2_ERR_HUB;
            }
        }

        if (!gotAck)
        {
            // Problem: try again.
            retries++;
        }
    }

    if (status >= 0)
    {
        status = SH2_OK;
    }

    return status;
}

// --- Private utility functions --------------------------------------------------------------


static void write32be(uint8_t *buf, uint32_t value)
{
    *buf++ = (value >> 24) & 0xFF;
    *buf++ = (value >> 16) & 0xFF;
    *buf++ = (value >> 8) & 0xFF;
    *buf++ = (value >> 0) & 0xFF;
}

static void appendCrc(uint8_t *packet, uint8_t len)
{
    uint16_t crc;
    uint16_t x;

    // compute CRC of packet
    crc = 0xFFFF;
    for (int n = 0; n < len; n++) {
        x = (uint16_t)(packet[n]) << 8;
        for (int i = 0; i < 8; i++) {
            if ((crc ^ x) & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            }
            else {
                crc = crc << 1;
            }
            x <<= 1;
        }
    }

    // Append the CRC to packet
    packet[len] = (crc >> 8) & 0xFF;
    packet[len+1] = crc & 0xFF;
}

int BnoDfu::sendAppSize(uint32_t appSize)
{
    write32be(dfuBuff, appSize);
    appendCrc(dfuBuff, 4);

    return dfuSend(dfuBuff, 6);
}

int BnoDfu::sendPktSize(uint8_t packetLen)
{
    dfuBuff[0] = packetLen;
    appendCrc(dfuBuff, 1);
    
    return dfuSend(dfuBuff, 3);
}

int BnoDfu::sendPkt(uint8_t* pData, uint32_t len)
{
    memcpy(dfuBuff, pData, len);
    appendCrc(dfuBuff, len);
    
    return dfuSend(dfuBuff, len+2);  // plus 2 for CRC
}

// Run DFU Process
bool BnoDfu::run(sh2_Hal_t *pHal_) {
    int rc;
    int status = SH2_OK;
    uint32_t appLen = 0;
    uint8_t packetLen = 0;
    uint32_t offset = 0;
    const char * s = 0;

    pHal = pHal_;

    // Open the hcbin object
    rc = firmware.open();
    if (rc != 0) {
        printf("Failed to open firmware.\n");
        status = SH2_ERR;
        goto end;
    }

    // Validate firmware matches this implementation
    s = firmware.getMeta("FW-Format");
    if ((s == 0) || (strcmp(s, "BNO_V1") != 0)) {
        // No format info or Incorrect format
        printf("No format or incorrect format firmware.\n");
        status = SH2_ERR_BAD_PARAM;
        goto close_and_return;
    }

    // Validate firmware is for the right part number
    s = firmware.getMeta("SW-Part-Number");
    if (s == 0) {
        // No part number info
        printf("No part number info for firmware.\n");
        status = SH2_ERR_BAD_PARAM;
        goto close_and_return;
    }
    if ((strcmp(s, "1000-3608") != 0) &&
        (strcmp(s, "1000-3676") != 0) &&
        (strcmp(s, "1000-4148") != 0) &&
        (strcmp(s, "1000-4563") != 0)) {
        // Incorrect part number
        printf("Incorrect part number info for firmware.\n");
        status = SH2_ERR_BAD_PARAM;
        goto close_and_return;
    }

    // Validate firmware length
    appLen = firmware.getAppLen();
    if (appLen < 1024) {
        // App data is empty
        printf("App data is empty, dummy firmware image?.\n");
        status = SH2_ERR_BAD_PARAM;
        goto close_and_return;
    }

    // Determine packet length to use
    packetLen = firmware.getPacketLen();
    if ((packetLen == 0) || (packetLen > MAX_PACKET_LEN)) {
        packetLen = MAX_PACKET_LEN;
    }

    // Initiate DFU process

    // Open the HAL instance used for DFU.
    status = pHal->open(pHal);
    if (status != SH2_OK) {
        printf("DFU Hal open returned error: %d\n", status);
        goto close_and_return;
    }

    // Send app size
    status = sendAppSize(appLen);
    if (status != SH2_OK) {
        goto close_and_return;
    }

    // Send packet size
    status = sendPktSize(packetLen);
    if (status != SH2_OK) {
        goto close_and_return;
    }
    
    // Send firmware image
    offset = 0;
    while (offset < appLen) {
        uint32_t toSend = appLen - offset;
        if (toSend > packetLen) {
            toSend = packetLen;
        }

        // Extract this packet's content from hcbin
        status = firmware.getAppData(dfuBuff, offset, toSend);
        if (status != SH2_OK) {
            goto close_and_return;
        }
        
        // Send this packet's contents
        status = sendPkt(dfuBuff, toSend);
        if (status != SH2_OK) {
            goto close_and_return;
        }

        // update loop variables
        offset += toSend;
    }

close_and_return:
    // close firmware
    firmware.close();

    // If update process completed successfully, delay a bit to let
    // flash writes complete.
    if (status == SH2_OK)
    {
        uint32_t now = pHal->getTimeUs(pHal);
        uint32_t start = now;
        while ((now - start) < DELAY_POST_DFU_US)
        {
            now = pHal->getTimeUs(pHal);
        }
    }

    // close device
    pHal->close(pHal);
    
end:
    // return true on success
    return (status == SH2_OK);;
}
