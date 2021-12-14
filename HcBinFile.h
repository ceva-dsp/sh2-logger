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

#ifndef HCBINFILE_H
#define HCBINFILE_H

#include "Firmware.h"

#include <string>
#include <vector>

typedef struct MetadataKV_s {
    std::string key;
    std::string value;
} MetadataKV_t;

class HcBinFile : public Firmware
{
  public:
    // Constructor
    HcBinFile(std::string filename);

  public:
    // Firmware interface
    virtual int open();
    virtual int close();
    virtual const char * getMeta(const char *key);
    virtual uint32_t getAppLen();
    virtual uint32_t getPacketLen();
    virtual int getAppData(uint8_t *packet, uint32_t offset, uint32_t len);

  private:
    // Private member functions
    int read32be(FILE * infile, uint32_t *pData);
    int readMetadata(FILE * infile, long int offset);
    int readPayload(FILE * infile);
    void updateCrc32(uint8_t c);
    
  private:
    // Private data
    std::string m_filename;
    bool m_open;
    uint32_t m_appLen;
    uint32_t m_crc32;
    uint32_t m_appDataLen;
    uint8_t *m_appData;
    std::vector<MetadataKV_t *> m_metadata;
};

#endif // #ifndef HCBINFILE_H
