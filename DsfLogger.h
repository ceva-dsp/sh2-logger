/*
 * Copyright 2018 Hillcrest Laboratories, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License and
 * any applicable agreements you may have with Hillcrest Laboratories, Inc.
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

#ifndef DSF_LOGGER_H
#define DSF_LOGGER_H

extern "C" {
#include "sh2.h"
#include "sh2_SensorValue.h"
}

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
// CLASS DEFINITON
// =================================================================================================
class DsfLogger {
public:
    DsfLogger(){};
    ~DsfLogger(){};

    bool init(char const* filePath, bool ned);
    void finish();

    void logMessage(char const* msg);
    void logAsyncEvent(sh2_AsyncEvent_t* pEvent, double timestamp);

    void logProductIds(sh2_ProductIds_t ids);
    void logFrsRecord(char const* name, uint32_t* buffer, uint16_t words);
    void logSensorValue(sh2_SensorValue_t* pValue, double timestamp);

private:
    // ---------------------------------------------------------------------------------------------
    // VARIABLES
    // ---------------------------------------------------------------------------------------------
    std::ofstream outFile_;
    bool orientationNed_;

    // ---------------------------------------------------------------------------------------------
    // PRIVATE METHODS
    // ---------------------------------------------------------------------------------------------
    double RadiansToDeg(float value);
    void WriteChannelDefinition(uint8_t sensorId, bool orientation = true);
    void WriteSensorReportHeader(sh2_SensorValue_t* pValue, SampleIdExtender* extender, double timestamp);
};

#endif // DSF_LOGGER_H
