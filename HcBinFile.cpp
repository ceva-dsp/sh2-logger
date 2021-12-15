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

#include <stdio.h>

extern "C" {
#include "sh2/sh2_err.h"
};


#define HCBIN_ID (0x6572d028)
#define HCBIN_FF_VER (4)
#define HCBIN_INIT_CRC (0xFFFFFFFF)

// Constructor
HcBinFile::HcBinFile(std::string filename)
{
    m_filename = filename;
    m_open = false;
    m_crc32 = 0;
    m_metadata.clear();
}

// Open file
int HcBinFile::open()
{
    int status = SH2_OK;

    uint32_t id = 0;
    uint32_t sz = 0;
    uint32_t ff_ver = 0;
    uint32_t offset = 0;
    long int pos = 0;
    uint32_t computed_crc = 0;
    uint32_t stored_crc = 0;
    

    if (m_open) {
        // Should not open the file while already open.
        return SH2_ERR;
    }

    // Initialize HcBinFile members
    m_appLen = 0;
    m_crc32 = HCBIN_INIT_CRC;
    m_appDataLen = 0;
    m_appData = 0;
    m_metadata.clear();

    // Open file
    FILE* infile = 0;
    infile = fopen(m_filename.c_str(), "rb");
    if (infile == 0) {
        // Error: could not open file
        status = SH2_ERR_BAD_PARAM;
        goto cleanup;
    }
    
    // Read into internal data structures
    // (on error, set status, goto cleanup so file will be closed)
    
    // read Id
    status = read32be(infile, &id);
    if (status != SH2_OK) goto cleanup;
    if (id != HCBIN_ID) {
        // Error: file isn't an HcBin file
        status = SH2_ERR_BAD_PARAM;
        goto cleanup;
    }

    // Read File size
    status = read32be(infile, &sz);
    if (status != SH2_OK) goto cleanup;

    // Read file format version
    status = read32be(infile, &ff_ver);
    if (status != SH2_OK) goto cleanup;
    if (ff_ver != HCBIN_FF_VER) {
        // This program doesn't understand this file format version
        status = SH2_ERR_BAD_PARAM;
        goto cleanup;
    }

    // Read payload offset
    status = read32be(infile, &offset);
    if (status != SH2_OK) goto cleanup;

    // Read Metadata entries
    status = readMetadata(infile, (long int)offset);
    if (status != SH2_OK) {
        // Something went wrong with metadata
        goto cleanup;
    }

    // get current file position
    pos = ftell(infile);
    if (pos < 0) goto cleanup;
    if (pos > (long int)offset) goto cleanup;

    // Skip anything else between metadata and offset
    // (But include it in the CRC32)
    while (pos < (long int)offset) {

        int c = fgetc(infile);
        updateCrc32(c);
        if (c == EOF) goto cleanup;

        pos += 1;
    }

    // Read payload data
    m_appDataLen = sz - offset - 4;  // app data len = file len - app data offset - len of final CRC32.
    status = readPayload(infile);
    if (status != SH2_OK) {
        // Something went wrong with payload
        goto cleanup;
    }

    // Read CRC
    computed_crc = ~m_crc32;
    status = read32be(infile, &stored_crc);
    if (status != SH2_OK) goto cleanup;
    if (stored_crc != computed_crc) {
        // CRC error on file contents
        status = SH2_ERR_BAD_PARAM;
        goto cleanup;
    }
  
    // No error, set open flag to true
    status = SH2_OK;
    m_open = true;

    // Close file
    if (infile != 0) {
        fclose(infile);
    }

    // Successful return.
    return SH2_OK;

cleanup:    
    close();
    if (infile != 0) {
        fclose(infile);
    }

    // Error return.
    return status;
}

int HcBinFile::close() {
    // Undo any storage allocations.
    if (m_appData) {
        delete[] m_appData;
    }

    // Clear metadata entries
    for (MetadataKV_t* kv : m_metadata) {
        delete kv;
    }
    m_metadata.clear();

    m_open = false;
    
    return 0;
}

const char* HcBinFile::getMeta(const char* key) {
    if (!m_open) return 0;
    
    for (MetadataKV_t* kv : m_metadata) {
        if (kv->key == key) {
            // Found the requested key
            return kv->value.c_str();
        }
    }

    // Not found.
    return 0;
}

uint32_t HcBinFile::getAppLen() {
    if (!m_open) return SH2_ERR;
    
    return m_appDataLen;
}

uint32_t HcBinFile::getPacketLen() {
    if (!m_open) return SH2_ERR;

    // No preferred packet length with this implementation.
    return 0;
}

int HcBinFile::getAppData(uint8_t* packet, uint32_t offset, uint32_t len) {
    if (!m_open) return SH2_ERR;
    if (offset+len > m_appDataLen) return SH2_ERR_BAD_PARAM;

    for (uint32_t copied = 0; copied < len; copied++) {
        packet[copied] = m_appData[offset+copied];
    }

    return SH2_OK;
}

int HcBinFile::read32be(FILE * infile, uint32_t *pData) {
    uint32_t retval = 0;

    for (int n = 0; n < 4; n++) {
        int c = fgetc(infile);
        updateCrc32(c);

        if (c == EOF) {
            // error
            return SH2_ERR_BAD_PARAM;
        }

        // pack into 32-bit retval, big endian style
        retval <<= 8;
        retval += (uint8_t)c;
    }

    *pData = retval;

    return SH2_OK;
}

int HcBinFile::readMetadata(FILE * infile, long int offset) {
    std::string key;
    std::string value;
    enum class ParseState {
        KEY, SEP, VALUE, EOL
    } state = ParseState::KEY;

    long int pos = ftell(infile);
    while (pos < offset) {
        bool ungot = false;
        int c = fgetc(infile);
        if (c == EOF) return SH2_ERR_BAD_PARAM;

        if (state == ParseState::KEY) {
            if (c == ':') {
                // key ends
                state = ParseState::SEP;
            }
            else {
                // add c to key
                key.append(1,c);
            }
        } else if (state == ParseState::SEP) {
            if (c == ' ') {
                // separator ends
                state = ParseState::VALUE;
            }
        } else if (state == ParseState::VALUE) {
            if ((c == '\n') || (c == '\r') || (c == '\0')) {
                // value ends
                state = ParseState::EOL;

                // store key/value pair
                MetadataKV_t *kv = new MetadataKV_t();
                kv->key = key;
                kv->value = value;
                m_metadata.push_back(kv);
                key.clear();
                value.clear();
            }
            else {
                // add c to value
                value.append(1,c);
            }
        }
        else {
            // in end of line
            if ((c != '\n') && (c != '\r') && (c != '\0')) {
                state = ParseState::KEY;
                ungetc(c, infile);
                ungot = true;
            }
        }

        if (!ungot) {
            updateCrc32(c);
            pos += 1;
        }
    }

    return SH2_OK;
}

int HcBinFile::readPayload(FILE *infile) {
    // Allocate appdata
    m_appData = new uint8_t[m_appDataLen];
    if (m_appData == 0) {
        return SH2_ERR;
    }

    for (uint32_t n = 0; n < m_appDataLen; n++) {
        int c = fgetc(infile);

        // update the CRC over the whole file
        updateCrc32(c);

        if (c == EOF) {
            // error
            return SH2_ERR_BAD_PARAM;
        }

        // pack into 32-bit retval, big endian style
        m_appData[n] = (uint8_t)c;

    }

    return SH2_OK;
}

void HcBinFile::updateCrc32(uint8_t c)
{
    for (int n = 0; n < 8; n++) {
        uint32_t b = (c ^ m_crc32) & 1;
        m_crc32 >>= 1;
        if (b)
            m_crc32 = m_crc32 ^ 0xEDB88320;
        c >>= 1;
    }
}
