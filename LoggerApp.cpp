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

#include "LoggerApp.h"
#include "LoggerUtil.h"
#include "DsfLogger.h"
#include "FtdiHal.h"
#include "TimerService.h"

#include "math.h"
#include <string.h>
#include <iomanip>
#include <iostream>
#include <string>


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

// =================================================================================================
// DATA TYPES
// =================================================================================================

// =================================================================================================
// LOCAL FUNCTION PROTOTYPES
// =================================================================================================
static int Sh2HalOpen(sh2_Hal_t* self);
static void Sh2HalClose(sh2_Hal_t* self);
static int Sh2HalRead(sh2_Hal_t* self, uint8_t* pBuffer, unsigned len, uint32_t* t_us);
static int Sh2HalWrite(sh2_Hal_t* self, uint8_t* pBuffer, unsigned len);
static uint32_t Sh2HalGetTimeUs(sh2_Hal_t* self);

// =================================================================================================
// LOCAL VARIABLES
// =================================================================================================
static TimerSrv* timer_;
static DsfLogger* logger_;
static FtdiHal* ftdiHal_;

static State_e state_ = State_e::Idle;

// Sensor Sample Timestamp
static bool useSampleTime_ = false;
static double firstSampleTime_us_ = 0;
static double currSampleTime_us_ = 0;
static double lastSampleTime_us_ = 0;

static uint64_t sensorEventsReceived_ = 0;

// =================================================================================================
// CONST LOCAL VARIABLES
// =================================================================================================
static sh2_Hal_t sh2Hal_ = {
	Sh2HalOpen,
	Sh2HalClose,
	Sh2HalRead,
	Sh2HalWrite,
	Sh2HalGetTimeUs,
};


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
        std::cout << "\nWARNING: SHTP error detected." << std::endl;
    }

    // Log ansync events
    logger_->logAsyncEvent(pEvent, currSampleTime_us_);
}

// -------------------------------------------------------------------------------------------------
// mySensorCallback
// -------------------------------------------------------------------------------------------------
void mySensorCallback(void* cookie, sh2_SensorEvent_t* pEvent) {
    sh2_SensorValue_t value;
    int rc = SH2_OK;

    rc = sh2_decodeSensorEvent(&value, pEvent);
    if (rc != SH2_OK) {
        return;
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
int LoggerApp::init(appConfig_s* appConfig, TimerSrv* timer, FtdiHal* ftdiHal, DsfLogger* logger) {
    int status;

    timer_ = timer;
    ftdiHal_ = ftdiHal;
    logger_ = logger;

    // ---------------------------------------------------------------------------------------------
    // Open SH2/SHTP connection
    // ---------------------------------------------------------------------------------------------
    state_ = State_e::Reset; // Enter 'Reset' state

    std::cout << "INFO: Open a session with a SensorHub \n";
    status = sh2_open(&sh2Hal_, myEventCallback, NULL);
    if (status != SH2_OK) {
        std::cout << "ERROR: Failed to open a SensorHub session : " << status << "\n";
        return -1;
    }

    // Stay in loop while waiting for system reset to complete
    int retryCnt = 3; // Number of retry allowed is 3
    while (true) {
		if (!WaitForResetComplete(RESET_TIMEOUT_)) {
            // Reset Timeout expired. Retry. 
            std::cout << "ERROR: Reset timeout. Retry ..\n";
            if (retryCnt-- <= 0) {
                return 1;
            }
            ftdiHal_->softreset();

        } else {
            break;
        }
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
    if (appConfig->clearDcd) {
        std::cout << "INFO: Clear DCD and Reset\n";
        uint32_t dummy = 0;
        // Clear DCD FRS record
        sh2_setFrs(DYNAMIC_CALIBRATION, &dummy, 0);

        // Clear DCD and Reset the target system
        state_ = State_e::Reset;
        sh2_clearDcdAndReset();
        // Wait for the system reset to complete
        if (!WaitForResetComplete(RESET_TIMEOUT_)) {
            std::cout << "ERROR: Failed to reset a SensorHub - Timeout \n";
            return -1;
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

        // std::cout << "INFO: Sensor ID : " << static_cast<uint32_t>(it->sensorId);
        // std::cout << " - " << SensorSpec_[it->sensorId].name;
        // std::cout << " @ " << (1e6 / config.reportInterval_us) << "Hz";
        // std::cout << " (" << config.reportInterval_us << "us)\n";
        sh2_setSensorConfig(it->sensorId, &config);
    }

    // Initialization Process complete
    // Transition to RUN state and observe sensor data
    firstReportReceived_ = false;
    state_ = State_e::Run;

    return 0;
}

// -------------------------------------------------------------------------------------------------
// LoggerApp::service
// -------------------------------------------------------------------------------------------------
int LoggerApp::service() {

    ReportProgress();

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
    for (sensorList_t::iterator it = pSensorsToEnable_->begin(); it != pSensorsToEnable_->end(); ++it) {
        sh2_setSensorConfig(it->sensorId, &config);
    }

    /*
    Sleep(100);
    logFrs(DYNAMIC_CALIBRATION, "dcdPre_Reset");

    sh2_reinitialize();

    logFrs(DYNAMIC_CALIBRATION, "dcdPostReset");
    */

    std::cout << "INFO: Closing the SensorHub session" << std::endl;
    sh2_close();        // Close SH2 driver
    logger_->finish();  // Close (DSF) Logger instance

    std::cout << "INFO: Shutdown complete" << std::endl;
    return 1;
}


// =================================================================================================
// LOCAL FUNCTIONS
// =================================================================================================
// -------------------------------------------------------------------------------------------------
// Sh2HalOpen
// -------------------------------------------------------------------------------------------------
static int Sh2HalOpen(sh2_Hal_t* self) {
    return ftdiHal_->open();
}

// -------------------------------------------------------------------------------------------------
// Sh2HalClose
// -------------------------------------------------------------------------------------------------
static void Sh2HalClose(sh2_Hal_t* self) {
    ftdiHal_->close();
}

// -------------------------------------------------------------------------------------------------
// Sh2HalRead
// -------------------------------------------------------------------------------------------------
static int Sh2HalRead(sh2_Hal_t* self, uint8_t* pBuffer, unsigned len, uint32_t* t_us) {
    return ftdiHal_->read(pBuffer, len, t_us);
}

// -------------------------------------------------------------------------------------------------
// Sh2HalWrite
// -------------------------------------------------------------------------------------------------
static int Sh2HalWrite(sh2_Hal_t* self, uint8_t* pBuffer, unsigned len) {
    return ftdiHal_->write(pBuffer, len);
}

// -------------------------------------------------------------------------------------------------
// Sh2HalGetTimeUs
// -------------------------------------------------------------------------------------------------
static uint32_t Sh2HalGetTimeUs(sh2_Hal_t* self) {
    return (uint32_t)timer_->getTimestamp_us();
}


// =================================================================================================
// PRIVATE METHODS
// =================================================================================================
// -------------------------------------------------------------------------------------------------
// LoggerApp::WaitForResetComplete
// -------------------------------------------------------------------------------------------------
bool LoggerApp::WaitForResetComplete(int loops) {
    std::cout << "INFO: Waiting for System Reset ";

    for (int resetCounter = 0; state_ == State_e::Reset; ++resetCounter) {
        // Print periodic progress indicator 
		if (resetCounter % 10000 == 9999) {
			std::cout << ".";
        }
        // Check resetCounter against 'loops' limit
        if (resetCounter >= loops) {
            std::cout << "\n";
            return false;
        }

        sh2_service();
    }
    // state_ has been updated before resetCounter reaches 'loops' limit
    return true;
}

// -------------------------------------------------------------------------------------------------
// LoggerApp::GetSensorConfiguration
// -------------------------------------------------------------------------------------------------
void LoggerApp::GetSensorConfiguration(sh2_SensorId_t sensorId, sh2_SensorConfig_t* pConfig) {
    memcpy(pConfig, LoggerUtil::SensorSpec[sensorId].config, sizeof(sh2_SensorConfig_t));
}

// -------------------------------------------------------------------------------------------------
// LoggerApp::ReportProgress
// -------------------------------------------------------------------------------------------------
void LoggerApp::ReportProgress() {
    uint64_t currSysTime_us;

    if (!firstReportReceived_) {
        firstReportReceived_ = true;
        lastReportTime_us_ = timer_->getTimestamp_us();
    }

    currSysTime_us = timer_->getTimestamp_us();

    if (currSysTime_us - lastReportTime_us_ >= 1000000) {
        lastReportTime_us_ = currSysTime_us;

        double deltaT = currSampleTime_us_ - firstSampleTime_us_;
        int32_t h = static_cast<int32_t>(floor(deltaT / 60.0 / 60.0));
        int32_t m = static_cast<int32_t>(floor((deltaT - h * 60 * 60) / 60.0));
        double s = deltaT - h * 60 * 60 - m * 60;

        std::cout << "Samples: " << std::setfill(' ') << std::setw(10) << sensorEventsReceived_
            << " Duration: " << h << ":" << std::setw(2) << std::setfill('0') << m << ":"
            << std::setprecision(2) << s << " "
            << " Rate: " << std::fixed << std::setprecision(2)
            << sensorEventsReceived_ / deltaT << " Hz" << std::endl;
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
        logger_->logFrsRecord(name, buffer, words);
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
    for (int i = 0; i < LoggerUtil::NumBno080FrsRecords; i++) {
        pFrs = &LoggerUtil::Bno080FrsRecords[i];
        LogFrsRecord(pFrs->recordId, pFrs->name);
    }
}
