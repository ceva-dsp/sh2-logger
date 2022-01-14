/*
 * Copyright 2018-21 CEVA, Inc.
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

#include "Logger.h"

#include <fstream>
#include <stdint.h>
#include <stddef.h>

// =================================================================================================
// CLASS DEFINITON - SampleIdExtender
// =================================================================================================
class SampleIdExtender {
public:
    SampleIdExtender() : empty_(true), seqMsb_(0), seqLsb_(0) {
    }

    uint64_t extend(uint8_t seq) {
        empty_ = false;
        if (seq < seqLsb_) {
            ++seqMsb_;
        }
        seqLsb_ = seq;
        return (seqMsb_ << 8) | seqLsb_;
    }

    bool isEmpty() {
        return empty_;
    }

private:
    bool empty_;
    uint64_t seqMsb_;
    uint8_t seqLsb_;
};


// =================================================================================================
// CLASS DEFINITON - DsfLogger
// =================================================================================================
class DsfLogger : public Logger {
public:
    DsfLogger(){};
    virtual ~DsfLogger(){};

    virtual bool init(char const* filePath, bool ned);
    virtual void finish();

    virtual void logMessage(char const* msg);
    virtual void logAsyncEvent(sh2_AsyncEvent_t* pEvent, double timestamp);

    virtual void logProductIds(sh2_ProductIds_t ids);
    virtual void logFrsRecord(char const* name, uint32_t* buffer, uint16_t words);
    virtual void logSensorValue(sh2_SensorValue_t* pValue, double timestamp, int64_t delay_uS);

private:
    // ---------------------------------------------------------------------------------------------
    // VARIABLES
    // ---------------------------------------------------------------------------------------------
    std::ofstream outFile_;

    // ---------------------------------------------------------------------------------------------
    // PRIVATE METHODS
    // ---------------------------------------------------------------------------------------------
    void WriteChannelDefinition(uint8_t sensorId, bool orientation = true);
    void WriteSensorReportHeader(sh2_SensorValue_t* pValue, SampleIdExtender* extender, double timestamp, int64_t delay_uS);
};
