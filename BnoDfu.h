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

#pragma once

extern "C" {
#include "sh2/sh2_hal.h"
}

#define MAX_PACKET_LEN (64)

class Firmware;

// DFU Process for BNO08x and related modules
class BnoDfu {
private:
    // Private Data
    sh2_Hal_t* pHal;
    uint8_t dfuBuff[MAX_PACKET_LEN + 2];

public:
    // Constructor
    BnoDfu();

private:
    // Private methods
    int dfuSend(uint8_t* pData, uint32_t len);
    int sendAppSize(uint32_t appSize);
    int sendPktSize(uint8_t packetLen);
    int sendPkt(uint8_t* pData, uint32_t len);

public:
    // Public API
    // Run DFU Process
    bool run(sh2_Hal_t* pHal, Firmware* firmware);
};
