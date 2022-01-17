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
extern "C" {
#include "sh2.h"
#include "sh2_SensorValue.h"
#include "sh2_err.h"
#include "sh2_hal.h"
}

#include "LoggerApp.h"
#include "LoggerUtil.h"
#include "Logger.h"

#include "math.h"
#include <string.h>
#include <iomanip>
#include <iostream>
#include <string>
#include <chrono>


// =================================================================================================
// DEFINES AND MACROS
// =================================================================================================
enum class State_e {
    Idle,    // Initial state
    Reset,   // Target device has been reset. Waiting for the startup response from the system.
    Startup, // Target device started up successfully. Ready to be configured.
    Run,     // Configurations complete. Start collecting sensor data.
};

#ifdef _WIN32
#define RESET_TIMEOUT_ 1000000
#else
#define RESET_TIMEOUT_ 1000
#endif

#define FLUSH_TIMEOUT 0.1f

// =================================================================================================
// DATA TYPES
// =================================================================================================

// =================================================================================================
// LOCAL VARIABLES
// =================================================================================================
static Logger* logger_;

static WheelSource* wheelSource_;

static State_e state_ = State_e::Idle;

// Sensor Sample Timestamp
static bool useSampleTime_ = false;
static double firstSampleTime_us_ = 0;
static double currSampleTime_us_ = 0;
static double lastSampleTime_us_ = 0;

static uint64_t sensorEventsReceived_ = 0;
static uint64_t lastSensorEventsReceived_ = 0;

static uint64_t shtpErrors_ = 0;

// =================================================================================================
// CONST LOCAL VARIABLES
// =================================================================================================

static sh2_Hal_t* sh2Hal_ = 0;


// =================================================================================================
// LOCAL FUNCTIONS
// =================================================================================================
// -------------------------------------------------------------------------------------------------
// myEventCallback
// -------------------------------------------------------------------------------------------------
void myEventCallback(void* cookie, sh2_AsyncEvent_t* pEvent) {

    switch (state_) {
        case State_e::Reset:
            if (pEvent->eventId == SH2_RESET) {
                std::cout << "\nINFO: Reset Complete" << std::endl;
                state_ = State_e::Startup;
            }
            break;
        default:
            break;
    }

    // Report SHTP errors
    if (pEvent->eventId == SH2_SHTP_EVENT) {
        shtpErrors_ += 1;
        
        // With latest SH2 implementation, one SHTP error,
        // for discarded advertisements, is normal.
        if (shtpErrors_ > 1) {
            std::cout << "\nWARNING: SHTP error detected." << std::endl;
        }
    }

    // Log async events
    logger_->logAsyncEvent(pEvent, currSampleTime_us_);
}

// -------------------------------------------------------------------------------------------------
// mySensorCallback
// -------------------------------------------------------------------------------------------------
void mySensorCallback(void* cookie, sh2_SensorEvent_t* pEvent) {
    static std::chrono::time_point<std::chrono::steady_clock> t0 = std::chrono::steady_clock::now();
    static bool ready = false;
    sh2_SensorValue_t value;
    int rc = SH2_OK;

    rc = sh2_decodeSensorEvent(&value, pEvent);
    if (!ready) {
        std::chrono::duration<double> dt = std::chrono::steady_clock::now() - t0;
        if (dt.count() > FLUSH_TIMEOUT){
            // Initial raw data samples may arrive out-of-order which
            // can result in invalid timestamps assignment. 
            ready = true;
        } else {
            return;
        }
    }
    if (rc != SH2_OK) {
        return;
    }
    if (wheelSource_ != nullptr) {
        wheelSource_->reportModuleTime(&value, pEvent);
    }

    if (useSampleTime_) {
        switch (value.sensorId) {
            case SH2_RAW_ACCELEROMETER:
                currSampleTime_us_ = value.un.rawAccelerometer.timestamp * 1e-6;
                lastSampleTime_us_ = currSampleTime_us_;
                break;
            case SH2_RAW_GYROSCOPE:
                currSampleTime_us_ = value.un.rawGyroscope.timestamp * 1e-6;
                lastSampleTime_us_ = currSampleTime_us_;
                break;
            case SH2_RAW_MAGNETOMETER:
                currSampleTime_us_ = value.un.rawMagnetometer.timestamp * 1e-6;
                lastSampleTime_us_ = currSampleTime_us_;
                break;
            case SH2_RAW_OPTICAL_FLOW:
                currSampleTime_us_ = value.un.rawOptFlow.timestamp * 1e-6;
                lastSampleTime_us_ = currSampleTime_us_;
                break;
            default:
                currSampleTime_us_ = lastSampleTime_us_;
                break;
        }
    } else {
        currSampleTime_us_ = value.timestamp * 1e-6;
    }

    if (firstSampleTime_us_ == 0) {
        firstSampleTime_us_ = currSampleTime_us_;
    }
    ++sensorEventsReceived_;

    // Log sensor data
    logger_->logSensorValue(&value, currSampleTime_us_);
}


// =================================================================================================
// PUBLIC FUNCTIONS
// =================================================================================================
// -------------------------------------------------------------------------------------------------
// LoggerApp::init
// -------------------------------------------------------------------------------------------------
int LoggerApp::init(appConfig_s* appConfig, sh2_Hal_t *pHal, 
                    Logger* logger, WheelSource* wheelSource) {
    int status;

    logger_ = logger;
    wheelSource_ = wheelSource;
    sh2Hal_ = pHal;

    // ---------------------------------------------------------------------------------------------
    // Open SH2/SHTP connection
    // ---------------------------------------------------------------------------------------------
    state_ = State_e::Reset; // Enter 'Reset' state
    shtpErrors_ = 0;

    std::cout << "INFO: Open a session with a SensorHub \n";
    status = sh2_open(sh2Hal_, myEventCallback, NULL);
    if (status != SH2_OK) {
        std::cout << "ERROR: Failed to open a SensorHub session : " << status << "\n";
        return -1;
    }

    // ---------------------------------------------------------------------------------------------
    // Set callback for Sensor Data
    // ---------------------------------------------------------------------------------------------
    status = sh2_setSensorCallback(mySensorCallback, NULL);
    if (status != SH2_OK) {
        std::cout << "ERROR: Failed to set Sensor callback\n";
        return -1;
    }

    // ---------------------------------------------------------------------------------------------
    // Clear DCD and Reset
    // ---------------------------------------------------------------------------------------------
    if (appConfig->clearDcd || appConfig->clearOfCal) {
        bool clearDcd = false;
        
        if (appConfig->clearOfCal) {
            std::cout << "INFO: Clear optical flow cal\n";
            uint32_t dummy = 0;
            sh2_setFrs(DR_CAL, &dummy, 0);
        }

        if (appConfig->clearDcd) {
            std::cout << "INFO: Clear DCD and Reset\n";
            // Clear DCD FRS record
            uint32_t dummy = 0;
            sh2_setFrs(DYNAMIC_CALIBRATION, &dummy, 0);

            // Clear DCD and Reset the target system
            state_ = State_e::Reset;
            clearDcd = true;
        }

        // Re-init, either with clearDcdAndReset or reinitialize
        if (clearDcd) {
            sh2_clearDcdAndReset();
        } else {
            sh2_reinitialize();
        }
            
    }

    // ---------------------------------------------------------------------------------------------
    // Get Product IDs
    // ---------------------------------------------------------------------------------------------
    std::cout << "INFO: Get Product IDs\n";
    sh2_ProductIds_t productIds;
    status = sh2_getProdIds(&productIds);
	if (status != SH2_OK) {
        std::cout << "ERROR: Failed to get product IDs\n";
        return -1;
    }
    logger_->logProductIds(productIds);

    // ---------------------------------------------------------------------------------------------
    // Set DCD Auto Save
    // ---------------------------------------------------------------------------------------------
    std::cout << "INFO: Set DCD Auto Save\n";
    status = sh2_setDcdAutoSave(appConfig->dcdAutoSave);
    if (status != SH2_OK) {
        std::cout << "ERROR: Failed to set DCD Auto Save\n";
        return -1;
    }

    // ---------------------------------------------------------------------------------------------
    // Set Calibration Configuration
    // ---------------------------------------------------------------------------------------------
    std::cout << "INFO: Set Calibration Configuration\n";
    status = sh2_setCalConfig(appConfig->calEnableMask);
    if (status != SH2_OK) {
        std::cout << "ERROR: Failed to set calibration configuration\n";
        return -1;
    }

    // ---------------------------------------------------------------------------------------------
    // Get Device FRS records
    // ---------------------------------------------------------------------------------------------
    std::cout << "INFO: Get FRS Records\n";
    LogAllFrsRecords();

    // ---------------------------------------------------------------------------------------------
    // Enable Sensors
    // ---------------------------------------------------------------------------------------------
    // Check if a config file is specified.
    pSensorsToEnable_ = appConfig->pSensorsToEnable;
    if (pSensorsToEnable_ == 0 || pSensorsToEnable_->empty()) {
        std::cout << "ERROR: NO Sensor list is specified.\n";
        return -1;
    }

    // Enable Sensors
    std::cout << "\nINFO: Enable Sensors\n";
    sh2_SensorConfig_t config;
    for (sensorList_t::iterator it = pSensorsToEnable_->begin(); it != pSensorsToEnable_->end(); ++it) {
        GetSensorConfiguration(it->sensorId, &config);
        config.reportInterval_us = it->reportInterval_us;
        config.sensorSpecific = it->sensorSpecific;
        config.sniffEnabled = it->sniffEnabled;

        sh2_setSensorConfig(it->sensorId, &config);

        if (appConfig->useRawSampleTime && IsRawSensor(it->sensorId)) {
            useSampleTime_ = true;
        }
    }

    // Initialization Process complete
    // Transition to RUN state and observe sensor data
    lastReportTime_us_ = 0;
    state_ = State_e::Run;

    return 0;
}

// -------------------------------------------------------------------------------------------------
// LoggerApp::service
// -------------------------------------------------------------------------------------------------
int LoggerApp::service() {

    ReportProgress();

    if (wheelSource_ != nullptr){
        wheelSource_->service();
    }

    sh2_service();

    return 1;
}

// -------------------------------------------------------------------------------------------------
// LoggerApp::finish
// -------------------------------------------------------------------------------------------------
int LoggerApp::finish() {

    // ---------------------------------------------------------------------------------------------
    // Turn off sensors
    // ---------------------------------------------------------------------------------------------
    std::cout << "INFO: Disable Sensors" << std::endl;
    sh2_SensorConfig_t config;
    memset(&config, 0, sizeof(config));
    config.reportInterval_us = 0;
    for (sensorList_t::iterator it = pSensorsToEnable_->begin();
         it != pSensorsToEnable_->end();
         ++it) {
        sh2_setSensorConfig(it->sensorId, &config);
    }

    // Save DCD
    std::cout << "INFO: Saving DCD." << std::endl;
    sh2_saveDcdNow();
    std::cout << "  Done." << std::endl;

    std::cout << "INFO: Closing the SensorHub session" << std::endl;
    sh2_close();        // Close SH2 driver
    logger_->finish();  // Close (DSF) Logger instance

    std::cout << "INFO: Shutdown complete" << std::endl;
    return 1;
}

// -------------------------------------------------------------------------------------------------
// LoggerApp::GetSensorConfiguration
// -------------------------------------------------------------------------------------------------
void LoggerApp::GetSensorConfiguration(sh2_SensorId_t sensorId, sh2_SensorConfig_t* pConfig) {
    memcpy(pConfig, LoggerUtil::SensorSpec[sensorId].config, sizeof(sh2_SensorConfig_t));
}

// -------------------------------------------------------------------------------------------------
// LoggerApp::IsRawSensor
// -------------------------------------------------------------------------------------------------
bool LoggerApp::IsRawSensor(sh2_SensorId_t sensorId) {
    if (sensorId == SH2_RAW_ACCELEROMETER || sensorId == SH2_RAW_GYROSCOPE ||
        sensorId == SH2_RAW_MAGNETOMETER) {
        return true;
    }
    else {
        return false;
    }
}

// -------------------------------------------------------------------------------------------------
// LoggerApp::ReportProgress
// -------------------------------------------------------------------------------------------------
void LoggerApp::ReportProgress() {
    uint64_t currSysTime_us;

    currSysTime_us = sh2Hal_->getTimeUs(sh2Hal_);

    if (currSysTime_us - lastReportTime_us_ >= 1000000) {

        double deltaT = currSampleTime_us_ - firstSampleTime_us_;
        int32_t h = static_cast<int32_t>(floor(deltaT / 60.0 / 60.0));
        int32_t m = static_cast<int32_t>(floor((deltaT - h * 60 * 60) / 60.0));
        double s = deltaT - h * 60 * 60 - m * 60;
        double last_window = (currSysTime_us - lastReportTime_us_)*1e-6;

        if (deltaT > 0) {
            std::cout << "Samples: " << std::setfill(' ') << std::setw(10) << sensorEventsReceived_
                      << " Duration: " << h << ":" << std::setw(2) << std::setfill('0') << m << ":"
                      << std::setw(2) << std::setfill('0') << int(s) << " "
                      << " Rate: " << std::fixed << std::setprecision(2)
                      << sensorEventsReceived_ / deltaT << " (" <<
                      (sensorEventsReceived_ - lastSensorEventsReceived_)/last_window
                      << ") Samples per second" << std::endl;
        }
        lastReportTime_us_ = currSysTime_us;
        lastSensorEventsReceived_ = sensorEventsReceived_;
    }
}

// -------------------------------------------------------------------------------------------------
// LoggerApp::LogFrsRecord
// -------------------------------------------------------------------------------------------------
int LoggerApp::LogFrsRecord(uint16_t recordId, char const* name) {
    uint32_t buffer[1024];
    memset(buffer, 0xAA, sizeof(buffer));
    uint16_t words = sizeof(buffer) / 4;

    int status = sh2_getFrs(recordId, buffer, &words);
    if (status != 0) {
        return 0;
    }

    if (words > 0) {
        logger_->logFrsRecord(recordId, name, buffer, words);
    }
    return words;
}

// -------------------------------------------------------------------------------------------------
// LoggerApp::LogAllFrsRecords
// -------------------------------------------------------------------------------------------------
void LoggerApp::LogAllFrsRecords() {
    if (LogFrsRecord(STATIC_CALIBRATION_AGM, "scd") == 0) {
        logger_->logMessage("# No SCD present, logging nominal calibration as 'scd'.");
        LogFrsRecord(NOMINAL_CALIBRATION, "scd");
    }

    const LoggerUtil::frsIdMap_s* pFrs;
    for (int i = 0; i < LoggerUtil::NumSh2FrsRecords; i++) {
        pFrs = &LoggerUtil::Sh2FrsRecords[i];
        LogFrsRecord(pFrs->recordId, pFrs->name);
    }
}
