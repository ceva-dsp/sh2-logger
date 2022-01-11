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

#include <stdint.h>
#include <stddef.h>

// =================================================================================================
// CLASS DEFINITON
// =================================================================================================
class ConsoleLogger : public Logger {
public:
    ConsoleLogger(){};
    virtual ~ConsoleLogger(){};

    virtual bool init(char const* filePath, bool ned);
    virtual void finish();

    virtual void logMessage(char const* msg);
    virtual void logAsyncEvent(sh2_AsyncEvent_t* pEvent, double currTime);

    virtual void logProductIds(sh2_ProductIds_t ids);
    virtual void logFrsRecord(char const* name, uint32_t* buffer, uint16_t words);
    virtual void logSensorValue(sh2_SensorValue_t* pValue, double currTime);

private:
    // ---------------------------------------------------------------------------------------------
    // VARIABLES
    // ---------------------------------------------------------------------------------------------
 
    // ---------------------------------------------------------------------------------------------
    // PRIVATE METHODS
    // ---------------------------------------------------------------------------------------------
    void WriteSensorReportHeader(sh2_SensorValue_t* pValue, double timestamp);
};

