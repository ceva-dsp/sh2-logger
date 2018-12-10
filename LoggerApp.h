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

#ifndef LOGGER_APP_H
#define LOGGER_APP_H

#include <list>
#include <stdint.h>

// =================================================================================================
// DEFINES AND MACROS
// =================================================================================================


// =================================================================================================
// DATA TYPES
// =================================================================================================
class TimerSrv;
class DsfLogger;
class FtdiHal;

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

        bool operator<(SensorFeatureSet_s const &other) { return sensorId < other.sensorId; }
        bool operator==(SensorFeatureSet_s const &other) { return sensorId == other.sensorId; }
    };
    typedef std::list<SensorFeatureSet_s> sensorList_t;

    struct appConfig_s {
        uint8_t calEnableMask = 0x0;
        bool clearDcd = false;
        bool dcdAutoSave = false;
        int deviceNumber = 0;
        bool orientationNed = true;
        sensorList_t * pSensorsToEnable = 0;
    };

    // ---------------------------------------------------------------------------------------------
    // PUBLIC METHODS
    // ---------------------------------------------------------------------------------------------
    int init(appConfig_s* appConfig, TimerSrv* timer, FtdiHal* ftdiHal, DsfLogger* logger);

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
    bool WaitForResetComplete(int loops);
    void GetSensorConfiguration(sh2_SensorId_t sensorId, sh2_SensorConfig_t* pConfig);
    void ReportProgress();

    int LogFrsRecord(uint16_t recordId, char const* name);
    void LogAllFrsRecords();
};

#endif // LOGGER_APP_H
