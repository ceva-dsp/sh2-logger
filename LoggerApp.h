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

#ifndef LOGGER_APP_H
#define LOGGER_APP_H

#include <list>
#include <stdint.h>
extern "C" {
    #include "sh2_hal.h"
}

// =================================================================================================
// DEFINES AND MACROS
// =================================================================================================


// =================================================================================================
// DATA TYPES
// =================================================================================================
class Logger;

// =================================================================================================
// CLASS DEFINITON - LoggerApp
// =================================================================================================
class LoggerApp {
public:
    LoggerApp(){};

    // ---------------------------------------------------------------------------------------------
    // DATA TYPES
    // ---------------------------------------------------------------------------------------------
    struct SensorFeatureSet_s {
        sh2_SensorId_t sensorId;
        uint32_t reportInterval_us;
        uint32_t sensorSpecific;
        uint32_t sniffEnabled;

        bool operator<(SensorFeatureSet_s const &other) { return sensorId < other.sensorId; }
        bool operator==(SensorFeatureSet_s const &other) { return sensorId == other.sensorId; }
        SensorFeatureSet_s():reportInterval_us(0),sensorSpecific(0),sniffEnabled(0){}
    };
    typedef std::list<SensorFeatureSet_s> sensorList_t;

    struct appConfig_s {
        bool dfuBno = false;
        bool dfuFsp = false;
        uint8_t calEnableMask = 0x0;
        bool clearDcd = false;
        bool clearOfCal = false;
        bool dcdAutoSave = false;
        bool orientationNed = true;
        sensorList_t * pSensorsToEnable = 0;
        int deviceNumber = 0;
        char deviceName[1024] = "";
    };

    // ---------------------------------------------------------------------------------------------
    // PUBLIC METHODS
    // ---------------------------------------------------------------------------------------------
    int init(appConfig_s* appConfig, sh2_Hal_t *pHal, Logger* logger);

    int service();

    int finish();

private:
    // ---------------------------------------------------------------------------------------------
    // VARIABLES
    // ---------------------------------------------------------------------------------------------
    sensorList_t * pSensorsToEnable_;
    uint64_t lastReportTime_us_;

    // ---------------------------------------------------------------------------------------------
    // PRIVATE METHODS
    // ---------------------------------------------------------------------------------------------
    void GetSensorConfiguration(sh2_SensorId_t sensorId, sh2_SensorConfig_t* pConfig);
    void ReportProgress();

    int LogFrsRecord(uint16_t recordId, char const* name);
    void LogAllFrsRecords();
};

#endif // LOGGER_APP_H
