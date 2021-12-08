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

/**
 * @file Firmware.h
 * @author David Wheeler
 * @date 7 December 2021
 * @brief API Definition for Firmware objects (Hillcrest HcBin Files).
 *
 * This interface definition is an abstraction of a firmware image in HcBin format.
 *
 */

#ifndef FIRMWARE_H
#define FIRMWARE_H

#include <stdint.h>

class Firmware {
  public:
    virtual int open() = 0;
    virtual int close() = 0;
    virtual const char * getMeta(const char *key) = 0;
    virtual uint32_t getAppLen() = 0;
    virtual uint32_t getPacketLen() = 0;
    virtual int getAppData(uint8_t *packet, uint32_t offset, uint32_t len) = 0;
};

#endif // #ifndef FIRMWARE_H
