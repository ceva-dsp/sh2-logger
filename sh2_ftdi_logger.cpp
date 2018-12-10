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

#define _CRT_SECURE_NO_WARNINGS

// =================================================================================================
// INCLUDE FILES
// =================================================================================================
#include "TimerService.h"
#include "DsfLogger.h"
#include "LoggerApp.h"
#include "LoggerUtil.h"

#ifdef _WIN32
#include "FtdiHalWin.h"
#else
#include "FtdiHalRpi.h"
#include "signal.h"
#endif

#include <string.h>
#include <iomanip>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

// =================================================================================================
// DEFINES AND MACROS
// =================================================================================================
using json = nlohmann::json;

// =================================================================================================
// DATA TYPES
// =================================================================================================

// =================================================================================================
// CONST LOCAL VARIABLES
// =================================================================================================
// JSON configuration file template
static const json loggerJson_ = {
    { "calEnable", "0x00" },
    { "clearDcd", false },
    { "dcdAutoSave", false },
    { "deviceNumber", 0 },
    { "orientation", "ned" },
    { "outDsfFile", "out.dsf" },
    { "sensorList", {
        { "Accelerometer", 0 },                         // 0x01
        { "Gyroscope", 0 },                             // 0x02
        { "Magnetic Field", 0 },                        // 0x03
        { "Linear Acceleration", 0 },                   // 0x04
        { "Rotation Vector", 0 },                       // 0x05
        { "Gravity", 0 },                               // 0x06
        { "Uncalibrated Gyroscope", 0 },                // 0x07
        { "Game Rotation Vector", 0 },                  // 0x08
        { "Geomagnetic Rotation Vector", 0 },           // 0x09
        { "Pressure", 0 },                              // 0x0A
        { "Ambient Light", 0 },                         // 0x0B
        { "Humidity", 0 },                              // 0x0C
        { "Proximity", 0 },                             // 0x0D
        { "Temperature", 0 },                           // 0x0E
        { "Uncalibrated MagneticField", 0 },            // 0x0F
        { "Tap Detector", 0 },                          // 0x10
        { "Step Counter", 0 },                          // 0x11
        { "Significant Motion", 0 },                    // 0x12
        { "Stability Classifier", 0 },                  // 0x13
        { "Raw Accelerometer", 0 },                     // 0x14
        { "Raw Gyroscope", 0 },                         // 0x15
        { "Raw Magnetometer", 0 },                      // 0x16
        { "Step Detector", 0 },                         // 0x18
        { "Shake Detector", 0 },                        // 0x19
        { "Flip Detector", 0 },                         // 0x1A
        { "Pickup Detector", 0 },                       // 0x1B
        { "Stability Detector", 0 },                    // 0x1C
        { "Personal Activity Classifier", 0 },          // 0x1E
        { "Sleep Detector", 0 },                        // 0x1F
        { "Tilt Detector", 0 },                         // 0x20
        { "Pocket Detector", 0 },                       // 0x21
        { "Circle Detector", 0 },                       // 0x22
        { "Heart Rate Monitor", 0 },                    // 0x23
        { "ARVR Stabilized Rotation Vector", 0 },       // 0x28
        { "ARVR Stabilized GameRotation Vector", 0 },   // 0x29
        { "Gyro Rotation Vector", 0 },                  // 0x2A
    } },
};

static const char DefaultJsonName[] = "logger.json";
static const uint32_t MaxPathLen = 260;

// =================================================================================================
// LOCAL VARIABLES
// =================================================================================================
#ifdef _WIN32
static TimerSrvWin timer_;
static FtdiHalWin ftdiHal_;
#else
static TimerSrvRpi timer_;
static FtdiHalRpi ftdiHal_;
#endif

static LoggerApp loggerApp_;
static DsfLogger dsfLogger_;
static bool runApp_ = true;

// Character array for output DSF file path
static char outDsfPath_[MaxPathLen];

// List of sensors to be enabled
static LoggerApp::sensorList_t sensorsToEnable_;

// Pointer to the batch json file path
static const char * pBatchFilePath_ = 0;


// =================================================================================================
// LOCAL FUNCTION PROTOTYPES
// =================================================================================================
bool ParseJsonBatchFile(LoggerApp::appConfig_s* pAppConfig);


// =================================================================================================
// LOCAL FUNCTIONS
// =================================================================================================
#ifndef _WIN32
void breakHandler(int) {
    fprintf(stderr, "break!\n");
    if (!runApp_) {
        fprintf(stderr, "force quit\n");
        exit(0);
    }
    runApp_ = false;
}
#endif

void usage(const char* myname) {
    fprintf(stderr,
            "\nUsage: %s <*.json> [--template]\n"
            "   *.json     - Configuration file in json format\n"
            "   --template - Generate a configuration template file, 'logger.json' \n",
            myname);
}


// ================================================================================================
// MAIN
// ================================================================================================
int main(int argc, const char* argv[]) {
    bool argError;
    LoggerApp::appConfig_s appConfig;
    json jBat;

#ifndef _WIN32
    struct sigaction act;
    act.sa_handler = breakHandler;
    sigaction(SIGINT, &act, NULL);
#endif

    // Clear local buffers
    memset(outDsfPath_, 0, sizeof(outDsfPath_));
    pBatchFilePath_ = 0;
    argError = true;

    // Process the argv[] arguments
    for (int i = 1, j = 0; i < argc; i++) {
        const char* arg = argv[i];
        if (arg[0] == '-') {
            // Generate json configuration file template
            if (strstr(arg, "--template") == arg) {
                // Write a configuration file template
                std::cout << "\nGenerate a configuration file template \"" << DefaultJsonName << "\". \n";
                std::ofstream ofile(DefaultJsonName);
                ofile << std::setw(4) << loggerJson_ << std::endl;
                // Exit after writing the template
                return 0;
            } else {
                break;
            }

        } else {
            // Process the 'batch' option. 
            if (strstr(arg, ".json")) {
                pBatchFilePath_ = arg;
                if (ParseJsonBatchFile(&appConfig)) {
                    argError = false;
                    break;
                } else {
                    return 0;
                }
            } else {
                break;
            }
        }
    }
 
    if (argError) {
        usage(argv[0]);
        return -1;
    }

    // --------------------------------------------------------------------------------------------
	// Start Application
	// --------------------------------------------------------------------------------------------
	runApp_ = true;

	// Initialize DSF Logger
    bool rv = dsfLogger_.init(outDsfPath_, appConfig.orientationNed);
    if (!rv) {
        std::cout << "ERROR: Unable to open dsf file:  \"" << outDsfPath_ << "\"" << std::endl;
        return -1;
    }

    // Initialize Timer
    timer_.init();

    // Initialze FTDI HAL
    int status;
    FtdiHal* pFtdiHal = &ftdiHal_;
    status = pFtdiHal->init(appConfig.deviceNumber, &timer_);
    if (status != 0) {
        std::cout << "ERROR: Initialize FTDI HAL failed!\n";
        return -1;
    }

    // Initialize the LoggerApp
    status = loggerApp_.init(&appConfig, &timer_, &ftdiHal_, &dsfLogger_);
    if (status != 0) {
        std::cout << "ERROR: Initialize LoggerApp failed!\n";
        return -1;
    }

#ifdef _WIN32
    HANDLE hstdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hstdin, &mode);
    SetConsoleMode(hstdin, 0);

    FlushConsoleInputBuffer(hstdin);
    std::cout << "\nPress a key to exit . . ." << std::endl;
#else

	std::cout << "\nPress Ctrl-C to exit . . ." << std::endl;
#endif

	std::cout << "\nProcessing Sensor Reports . . ." << std::endl;

	uint64_t currSysTime_us = timer_.getTimestamp_us();
	uint64_t lastChecked_us = currSysTime_us;

    while (runApp_) {

#ifdef _WIN32
        currSysTime_us = timer_.getTimestamp_us();
		
		if (currSysTime_us - lastChecked_us > 200000) {
            lastChecked_us = currSysTime_us;

            INPUT_RECORD event;
            DWORD count;

            PeekConsoleInput(hstdin, &event, 1, &count);
            if (count) {
                ReadConsoleInput(hstdin, &event, 1, &count);
                if ((event.EventType == KEY_EVENT) && !event.Event.KeyEvent.bKeyDown) {
					runApp_ = false;
					break;
                }
            }
        }
#endif

        loggerApp_.service();
    }

    std::cout << "\nINFO: Shutting down" << std::endl;

    loggerApp_.finish();

#ifdef _WIN32
    SetConsoleMode(hstdin, mode);
#endif
    return 0;
}


// ================================================================================================
// ParseJsonBatchFile
// ================================================================================================
bool ParseJsonBatchFile(LoggerApp::appConfig_s* pAppConfig) {
    json jBat;
    bool foundSensorList = false;

    std::cout << "\nINFO: (json) Process the batch json file \'" << pBatchFilePath_  << "\' ... \n";

    // Batch file is specified, extract the configuration options.
    std::ifstream ifile(pBatchFilePath_);

    // Read batch file into JSON object
    try {
        ifile >> jBat;
    } catch (...) {
        std::cout << "\nERROR: Json parser error. Abort!\n";
        return false;
    }

    // iterate through the json object
    for (json::iterator it = jBat.begin(); it != jBat.end(); ++it) {

        if (it.key().compare("calEnable") == 0) {
            std::string s = it.value();
            pAppConfig->calEnableMask = static_cast<uint8_t>(strtoul(s.c_str(), nullptr, 16));
            std::cout << "INFO: (json) Calibration Enable : " << static_cast<uint32_t>(pAppConfig->calEnableMask) << "\n";

        } else if (it.key().compare("clearDcd") == 0) {
            pAppConfig->clearDcd = it.value();
            std::cout << "INFO: (json) Clear DCD : ";
            if (pAppConfig->clearDcd) {
                std::cout << "Enable\n";
            } else {
                std::cout << "Disable\n";
            }

        } else if (it.key().compare("dcdAutoSave") == 0) {
            pAppConfig->dcdAutoSave = it.value();
            std::cout << "INFO: (json) DCD Auto Save : ";
            if (pAppConfig->dcdAutoSave) {
                std::cout << "Enable\n";
            } else {
                std::cout << "Disable\n";
            }

        } else if (it.key().compare("deviceNumber") == 0) {
            pAppConfig->deviceNumber = it.value();
            std::cout << "INFO: (json) Device Number : " << pAppConfig->deviceNumber << "\n";

        } else if (it.key().compare("orientation") == 0) {
            std::string val = it.value();
            std::cout << "INFO: (json) Orientation : "; 
            if (val.compare("enu") == 0) {
                pAppConfig->orientationNed = false;
                std::cout << "ENU\n";
            } else {
                pAppConfig->orientationNed = true;
                std::cout << "NED\n";
            }

        } else if (it.key().compare("outDsfFile") == 0) {
            std::string val = it.value();
            if (val.size() < MaxPathLen) {
                memcpy(outDsfPath_, val.c_str(), val.size());
                std::cout << "INFO: (json) DSF Output file : " << outDsfPath_ << "\n";
            } else {
                std::cout << "ERROR: The length of the output DSF file path exceeds the limit (max 260 characters). Abort. \n";
                return false;
            }

        } else if (it.key().compare("sensorList") == 0) {
            foundSensorList = true;

            std::cout << "INFO: (json) Extract Sensor list ... \n";

            for (json::iterator sl = it.value().begin(); sl != it.value().end(); ++sl) {
                LoggerApp::SensorFeatureSet_s config;
                config.sensorId = (sh2_SensorId_t)LoggerUtil::findSensorIdByName(sl.key().c_str());
                float rate = sl.value();
                config.reportInterval_us = static_cast<uint32_t>((1e6 / rate) + 0.5);

                // Add the new sensor to sensorsToEnable_ if the ID is valid.
                if (config.sensorId <= SH2_MAX_SENSOR_ID && config.reportInterval_us > 0) {
                    std::cout << "INFO: (json)      Sensor ID : " << static_cast<uint32_t>(config.sensorId);
                    std::cout << " - " << sl.key();
                    std::cout << " @ " << (1e6 / config.reportInterval_us) << "Hz";
                    std::cout << " (" << config.reportInterval_us << "us)\n";

                    sensorsToEnable_.push_back(config);
                }
            }

            sensorsToEnable_.sort();
            sensorsToEnable_.unique();

            if (sensorsToEnable_.empty()) {
                std::cout << "ERROR: No sensor is enabled. Abort.\n";
                return false;
            }

            pAppConfig->pSensorsToEnable = &sensorsToEnable_;
        }
    }

    if (!foundSensorList) {
        std::cout << "\nERROR: \"sensorList\" is not specified in the json file. Abort. \n";
        return false;
    }

    // If an output dsf file path is not specified, return error then abort.
    if (strlen(outDsfPath_) == 0) {
        std::cout << "\nERROR: Output DSF file path is not specified. Abort. \n";
        return false;
    }

    std::cout << "\n";
    return true;
}
