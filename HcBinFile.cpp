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
 * @file HcBinFile.h
 * @author David Wheeler
 * @date 7 December 2021
 * @brief API Definition for Firmware objects (Hillcrest HcBin Files).
 *
 * This class implements the Firmware interface for .hcbin files.
 *
 */

#include "HcBinFile.h"

// Constructor
HcBinFile::HcBinFile(std::string filename)
{
    m_filename = filename;
    m_open = false;
    
    // TODO
}

// Open file
int HcBinFile::open()
{

    // TODO
    return 0;
}

int HcBinFile::close() {
    // TODO
    return 0;
}

const char* HcBinFile::getMeta(const char* key) {
    // TODO
    return "Hi Mom";
}

uint32_t HcBinFile::getAppLen() {
    // TODO
    return 0;
}

uint32_t HcBinFile::getPacketLen() {
    // TODO
    return 0;
}

int HcBinFile::getAppData(uint8_t* packet, uint32_t offset, uint32_t len) {
    // TODO
    return 0;
}

