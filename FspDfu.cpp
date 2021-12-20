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

#include "FspDfu.h"

#include <string.h>

extern "C" {
    #include "sh2/sh2_err.h"
    #include "sh2/shtp.h"
}

#define CHAN_BOOTLOADER_CONTROL (1)

// DFU should complete in about 36 seconds.  Set timeout at 60.
#define DFU_TIMEOUT_US (240000000)  // Can take up to 240 sec at 9600 baud.

// Bootloader message ids
typedef enum {
    ID_PRODID_REQ  = 0xe1,
    ID_PRODID_RESP = 0xe2,
    ID_OPMODE_REQ  = 0xe3,
    ID_OPMODE_RESP = 0xe4,
    ID_STATUS_REQ  = 0xe5,
    ID_STATUS_RESP = 0xe6,
    ID_WRITE_REQ   = 0xe7,
    ID_WRITE_RESP  = 0xe8,
} ReportId_t;

// Bootloader operating modes
typedef enum {
    OPMODE_BOOTLOADER,
    OPMODE_UPGRADE,
    OPMODE_VALIDATE,
    OPMODE_APPLICATION,
} OpMode_t;

// Flags in status word
#define STATUS_LAUNCH_APPLICATION 0x00000001
#define STATUS_LAUNCH_BOOTLOADER  0x00000002
#define STATUS_UPGRADE_STARTED    0x00000004
#define STATUS_VALIDATE_STARTED   0x00000008
#define STATUS_APP_VALID          0x00000010
#define STATUS_APP_INVALID        0x00000020
#define STATUS_DFU_IMAGE_VALID    0x00000040
#define STATUS_DFU_IMAGE_INVALID  0x00000080
#define STATUS_ERROR              0x40000000
#define STATUS_SOURCE             0x80000000

// Error codes
typedef enum {
    DFU_NO_ERROR = 0,
    DFU_UNEXPECTED_CMD,
    DFU_INVALID_APPLICATION,
    DFU_FLASH_ERASE_ERR,
    DFU_FLASH_WRITE_ERR,
    DFU_FLASH_LOCK_ERR,
    DFU_FLASH_OVERFLOW,
    DFU_INVALID_IMAGE_TYPE,
    DFU_INVALID_IMAGE_SIZE,
    DFU_INVALID_IMAGE_VERSION,
    DFU_INCOMPATIBLE_HARDWARE,
    DFU_RESERVED_0B,
    DFU_RESERVED_0C,
    DFU_IMAGE_LEN_MISMATCH,
    DFU_INVALID_APP_SIZE_DFU_IMAGE,
    DFU_INVALID_APP_CRC_DFU_IMAGE,
    DFU_INVALID_IMAGE_CRC,
    DFU_INVALID_PAYLOAD_LENGTH,
    DFU_INVALID_DATA_OFFSET,
} DfuErr_t;

// Constructor
FspDfu::FspDfu() {
    initState();
}

void FspDfu::initState() {
    m_status = SH2_OK;
    m_firmwareOpened = false;
    m_appLen = 0;
    m_ignoredResponses = 0;
    m_state = ST_INIT;
}

void FspDfu::openFirmware() {
    if (m_firmware == 0) {
        // No firmware object
        m_status = SH2_ERR_BAD_PARAM;
        return ;
    }

    // Open the hcbin object
    int rc = m_firmware->open();
    if (rc != 0) {
        // Could not open firmware file
        m_status = SH2_ERR;
        return;
    }
    m_firmwareOpened = true;

    // Validate firmware matches this implementation
    const char *s = m_firmware->getMeta("FW-Format");
    if ((s == 0) || (strcmp(s, "EFM32_V1") != 0)) {
        // No format info or Incorrect format
        m_status = SH2_ERR_BAD_PARAM;
        return;
    }

    // Validate firmware is for the right part number
    s = m_firmware->getMeta("SW-Part-Number");
    if (s == 0) {
        // No part number info
        m_status = SH2_ERR_BAD_PARAM;
        return;
    }
    if (strcmp(s, "1000-4095") != 0) {
        // Incorrect part number
        m_status = SH2_ERR_BAD_PARAM;
        return;
    }

    // Validate firmware length
    m_appLen = m_firmware->getAppLen();
    if (m_appLen < 1024) {
        // App data isn't real
        m_status = SH2_ERR_BAD_PARAM;
        return;
    }
    if ((m_appLen & 0x00000003) != 0) {
        // appLen should be a multiple of 4.
        m_status = SH2_ERR_BAD_PARAM;
        return;
    }
}

static uint32_t getU32(uint8_t *payload, unsigned offset)
{
    uint32_t value = 0;

    value =
        (payload[offset]) +
        (payload[offset+1] << 8) +
        (payload[offset+2] << 16) +
        (payload[offset+3] << 24);

    return value;
}

static uint16_t getU16(uint8_t *payload, unsigned offset)
{
    uint16_t value = 0;

    value =
        (payload[offset]) +
        (payload[offset+1] << 8);

    return value;
}

typedef struct {
    uint8_t reportId;
    uint8_t opMode;
} OpModeRequest_t;

void FspDfu::requestUpgrade()
{
    OpModeRequest_t req;

    req.reportId = ID_OPMODE_REQ;
    req.opMode = OPMODE_UPGRADE;
    
    shtp_send(m_pShtp, CHAN_BOOTLOADER_CONTROL, (uint8_t *)&req, sizeof(req));
}

DfuState_t FspDfu::handleInitStatus(uint8_t *payload, uint16_t len)
{
    DfuState_t nextState = m_state;
    uint32_t status = getU32(payload, 4);
    uint32_t errCode = getU32(payload, 8);

    // Make sure status says we can proceed.
    if (status & STATUS_LAUNCH_BOOTLOADER) {
        // Issue request to start download
        requestUpgrade();
        nextState = ST_SETTING_MODE;
    }
    else {
        // Can't start
        m_status = SH2_ERR_HUB;
        nextState = ST_FINISHED;
    }
    
    return nextState;
}

typedef struct {
    uint8_t reportId;
    uint8_t length;
    uint8_t wordOffset_lsb;
    uint8_t wordOffset_msb;
    uint8_t data[16*4];
} WriteRequest_t;

void FspDfu::requestWrite()
{
    WriteRequest_t req;

    // How many words to write next
    m_writeLen = (m_appLen/4) - m_wordOffset;
    if (m_writeLen > 16) {
        m_writeLen = 16;
    }

    req.reportId = ID_WRITE_REQ;
    req.length = m_writeLen;
    req.wordOffset_lsb = m_wordOffset & 0xFF;
    req.wordOffset_msb = (m_wordOffset >>8) & 0xFF;
    m_firmware->getAppData(req.data, m_wordOffset*4, m_writeLen*4);
    
    shtp_send(m_pShtp, CHAN_BOOTLOADER_CONTROL, (uint8_t *)&req, sizeof(req));
}

DfuState_t FspDfu::handleModeResponse(uint8_t *payload, uint16_t len)
{
    DfuState_t nextState = m_state;
    uint8_t opMode = payload[1];
    uint8_t opModeStatus = payload[2];

    // Make sure transition to upgrade mode succeeded
    if ((opMode == OPMODE_UPGRADE) &&
        (opModeStatus == 0)) {
        m_wordOffset = 0;
        requestWrite();
        nextState = ST_SENDING_DATA;
    }
    else {
        // Failed to start upgrade mode
        m_status = SH2_ERR_HUB;
        nextState = ST_FINISHED;
    }
    
    return nextState;
}

DfuState_t FspDfu::handleWriteResponse(uint8_t *payload, uint16_t len)
{
    DfuState_t nextState = m_state;
    uint8_t writeStatus = payload[1];
    uint16_t wordOffset = getU16(payload, 2);

    if (writeStatus == 0) {
        m_wordOffset += m_writeLen;
        if (m_wordOffset*4 == m_appLen) {
            // Now we wait for final status update
            nextState = ST_WAIT_COMPLETION;
        }
        else {
            // update offset and issue new write
            requestWrite();
            nextState = ST_SENDING_DATA;
        }
    }
    else {
        // Errored out
        m_status = SH2_ERR_HUB;
        nextState = ST_FINISHED;
    }

    return nextState;
}

void FspDfu::requestLaunch()
{
    OpModeRequest_t req;

    req.reportId = ID_OPMODE_REQ;
    req.opMode = OPMODE_APPLICATION;
    
    shtp_send(m_pShtp, CHAN_BOOTLOADER_CONTROL, (uint8_t *)&req, sizeof(req));
}

DfuState_t FspDfu::handleFinalStatus(uint8_t *payload, uint16_t len)
{
    DfuState_t nextState = m_state;

    uint32_t status = getU32(payload, 4);
    uint32_t errCode = getU32(payload, 8);

    if ((status & STATUS_APP_VALID) &&
        ((status & STATUS_ERROR) == 0) &&
        (errCode == DFU_NO_ERROR)) {
        requestLaunch();
        m_status = SH2_OK;
        nextState = ST_LAUNCHING;
    }
    else {
        m_status = SH2_ERR_HUB;
        nextState = ST_FINISHED;
    }

    return nextState;
}

DfuState_t FspDfu::handleLaunchResp(uint8_t *payload, uint16_t len)
{
    DfuState_t nextState = m_state;

    uint8_t opMode = payload[1];
    uint8_t opModeStatus = payload[2];

    if ((opMode == OPMODE_APPLICATION) &&
        (opModeStatus == 0)) {
        m_status = SH2_OK;
        nextState = ST_FINISHED;
    }
    else {
        m_status = SH2_ERR_HUB;
        nextState = ST_FINISHED;
    }

    return nextState;
}

const DfuTransition_t FspDfu::dfuStateTransition[] = {
    {ST_INIT, ID_STATUS_RESP, &FspDfu::handleInitStatus},
    {ST_SETTING_MODE, ID_OPMODE_RESP, &FspDfu::handleModeResponse},
    {ST_SENDING_DATA, ID_WRITE_RESP, &FspDfu::handleWriteResponse},
    {ST_WAIT_COMPLETION, ID_STATUS_RESP, &FspDfu::handleFinalStatus},
    {ST_LAUNCHING, ID_OPMODE_RESP, &FspDfu::handleLaunchResp},
};

const DfuTransition_t *FspDfu::findTransition(DfuState_t state, uint8_t reportId)
{
    for (int n = 0; n < sizeof(dfuStateTransition)/sizeof(dfuStateTransition[0]); n++) {
        if ((dfuStateTransition[n].state == state) &&
            (dfuStateTransition[n].reportId == reportId)) {
            // Found the entry for this state, reportId
            return &dfuStateTransition[n];
        }
    }

    // Didn't find a match
    return 0;
}

// CHAN_BOOTLOADER_CONTROL handler
void FspDfu::bootloader_ctrl_hdlr(uint8_t *payload, uint16_t len, uint32_t timestamp)
{
    uint8_t reportId = payload[0];

    // Find a state machine table entry matching current state and report id.
    const DfuTransition_t *pEntry = findTransition(m_state, reportId);

    if (pEntry) {
        // Take the prescribed action for this transition and assign new state
        DfuAction_t action = pEntry->action;
        m_state = (this->*action)(payload, len);
    }
    else {
        // Unexpected event/state combination.  Ignore.
        m_ignoredResponses++;
    }
}    

// CHAN_BOOTLOADER_CONTROL handler
static void hdlr(void *cookie, uint8_t *payload, uint16_t len, uint32_t timestamp)
{
    // Static function calls member function by casting cookie to (FspDfu *)
    
    FspDfu *self = (FspDfu *)cookie;
    self->bootloader_ctrl_hdlr(payload, len, timestamp);
}
    
// Run DFU Process
bool FspDfu::run(sh2_Hal_t *pHal_, Firmware *firmware) {
    uint32_t start_us = 0;
    uint32_t now_us = 0;

    // Store Hal pointer as a member
    m_pHal = pHal_;

    // initialize state.
    // (The DFU process runs, mainly in the context of the CHAN_BOOTLOADER_CONTROL channel listener.
    initState();
    
    // Open firmware and validate it
    m_firmware = firmware;
    openFirmware();
    if (m_status != SH2_OK) goto fin;
    
    // Initialize SHTP layer
    m_pShtp = shtp_open(m_pHal);
    if (m_pShtp == 0) {
        return false;
    }

    // register channel handlers for DFU
    shtp_listenChan(m_pShtp, CHAN_BOOTLOADER_CONTROL, hdlr, this);

    // service SHTP until DFU process completes
    now_us = m_pHal->getTimeUs(m_pHal);
    start_us = now_us;
    while (((now_us-start_us) < DFU_TIMEOUT_US) &&
           (m_state != ST_FINISHED)) {
        shtp_service(m_pShtp);
        now_us = m_pHal->getTimeUs(m_pHal);
    }

    if (m_state != ST_FINISHED) {
        m_status = SH2_ERR_TIMEOUT;
    }

    // close SHTP
    shtp_close(m_pShtp);
    m_pShtp = 0;
    
fin:
    if (m_firmwareOpened) {
        m_firmware->close();
        m_firmwareOpened = false;
    }

    return (m_status == SH2_OK);
}
