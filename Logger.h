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

#ifndef LOGGER_H
#define LOGGER_H

extern "C" {
#include "sh2.h"
#include "sh2_SensorValue.h"
}

#include <fstream>
#include <stdint.h>
#include <stddef.h>

// =================================================================================================
// CLASS DEFINITON
// =================================================================================================
class Logger {
public:
    Logger(){};
    virtual ~Logger(){};

    virtual bool init(char const* filePath, bool ned) = 0;
    virtual void finish() = 0;

    virtual void logMessage(char const* msg) = 0;
    virtual void logAsyncEvent(sh2_AsyncEvent_t* pEvent, double timestamp) = 0;

    virtual void logProductIds(sh2_ProductIds_t ids) = 0;
    virtual void logFrsRecord(uint16_t recordId, char const* name, uint32_t* buffer, uint16_t words) = 0;
    virtual void logSensorValue(sh2_SensorValue_t* pValue, double timestamp) = 0;

protected:
    // ---------------------------------------------------------------------------------------------
    // VARIABLES
    // ---------------------------------------------------------------------------------------------
    bool orientationNed_;

    // ---------------------------------------------------------------------------------------------
    // PROTECTED METHODS
    // ---------------------------------------------------------------------------------------------
    double RadiansToDeg(float value) {
        double const PI = 3.1415926535897932384626433832795;
        return (value * 180.0) / PI;
    }
};

#endif // LOGGER_H
