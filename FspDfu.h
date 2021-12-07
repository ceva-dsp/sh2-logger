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

#ifndef FSPDFU_H
#define FSPDFU_H

#include "sh2/sh2_hal.h"

extern "C" {
    #include "firmware-fsp200.h"
}

#define MAX_PACKET_LEN (64)

// ------------------------------------------------------------------------
// Type definitions

class FspDfu;

// States of DFU process
typedef enum {
    ST_INIT,
    ST_SETTING_MODE,
    ST_SENDING_DATA,
    ST_WAIT_COMPLETION,
    ST_LAUNCHING,
    ST_FINISHED,
} DfuState_t;

// DFU State machine Action typedef
typedef DfuState_t (FspDfu::* DfuAction_t)(uint8_t *payload, uint16_t len);

// DFU State machine transition typedef
typedef struct {
    DfuState_t state;
    uint8_t reportId;
    DfuAction_t action;
} DfuTransition_t;

static void hdlr(void *cookie, uint8_t *payload, uint16_t len, uint32_t timestamp);

// DFU Process for FSP200 and similar modules
class FspDfu {
    friend void hdlr(void *cookie, uint8_t *payload, uint16_t len, uint32_t timestamp);

    
  private:
    static const DfuTransition_t dfuStateTransition[];

  private:
    // Private Data
    sh2_Hal_t *m_pHal;
    void *m_pShtp;
    int m_status;
    bool m_firmwareOpened;
    const HcBin_t *m_firmware;
    uint32_t m_appLen;

    uint16_t m_wordOffset;
    uint16_t m_writeLen;
    
    uint32_t m_ignoredResponses;
    DfuState_t m_state;
    
  public:
    // Constructor
    FspDfu();

  private:
    // Private methods
    void requestUpgrade();
    DfuState_t handleInitStatus(uint8_t *payload, uint16_t len);
    void requestWrite();
    DfuState_t handleModeResponse(uint8_t *payload, uint16_t len);
    DfuState_t handleWriteResponse(uint8_t *payload, uint16_t len);
    void requestLaunch();
    DfuState_t handleFinalStatus(uint8_t *payload, uint16_t len);
    DfuState_t handleLaunchResp(uint8_t *payload, uint16_t len);
    const DfuTransition_t *findTransition(DfuState_t state, uint8_t reportId);


    void initState();
    void openFirmware();
    void bootloader_ctrl_hdlr(uint8_t *payload, uint16_t len, uint32_t timestamp);

  public:
    // Run DFU Process
    bool run(sh2_Hal_t *pHal);
};

#endif // ifndef FSPDFU_H
