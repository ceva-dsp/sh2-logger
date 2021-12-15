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

#define _CRT_SECURE_NO_WARNINGS

// =================================================================================================
// INCLUDE FILES
// =================================================================================================
#include <string.h>
#include <iomanip>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include "tclap/CmdLine.h"

/// #include "TimerService.h"
#include "DsfLogger.h"
#include "ConsoleLogger.h"
#include "LoggerApp.h"
#include "LoggerUtil.h"
#include "FspDfu.h"
#include "BnoDfu.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#ifndef _WIN32
#include "signal.h"
#endif

#include "HcBinFile.h"

extern "C" {
#include "bno_dfu_hal.h"
#include "ftdi_hal.h"
}

// =================================================================================================
// DEFINES AND MACROS
// =================================================================================================
using json = nlohmann::json;

// =================================================================================================
// DATA TYPES
// =================================================================================================

// =================================================================================================
// LOCAL FUNCTION PROTOTYPES
// =================================================================================================
bool ParseJsonBatchFile(std::string inFilename, LoggerApp::appConfig_s* pAppConfig);


// =================================================================================================
// CONST LOCAL VARIABLES
// =================================================================================================
static const uint32_t MaxPathLen = 260;

// =================================================================================================
// LOCAL VARIABLES
// =================================================================================================

// Is set to false in Break handler to stop application gracefully.
volatile static bool runApp_ = true;

class Sh2Logger {
  public:
    Sh2Logger() { /* nothing to do */ }

  public:
    void parseArgs(int argc, const char *argv[]);
    int run();
    int do_template();
    int do_logging();
    int do_dfu_bno();
    int do_dfu_fsp();
    
  private:
    std::string m_cmd;
    
    bool m_outFilenameSet;
    std::string m_outFilename;
    
    bool m_inFilenameSet;
    std::string m_inFilename;
    
    bool m_deviceArgSet;
#ifdef _WIN32
    unsigned m_deviceArg;
#else
    std::string m_deviceArg;
#endif

    bool m_clearDcd;
    bool m_clearOfCal;
};

void Sh2Logger::parseArgs(int argc, const char *argv[]) {
    // Process command line args
    TCLAP::CmdLine cmd("SH2 Logging utility", ' ', "1.0");

    // Command: log (default), dfu,
    std::vector<std::string> operations = {"log", "dfu-bno", "dfu-fsp200", "template"};
    TCLAP::ValuesConstraint<std::string> opConstr(operations);
    TCLAP::UnlabeledValueArg<std::string> cmdArg("command", "Operation to perform", true, "log", &opConstr);
    cmd.add(cmdArg);

    // --input
    // optional, This is the .json filename for logging or a firmware file for DFU.
    TCLAP::ValueArg<std::string> inFilenameArg("i", "input", 
                                               "Input filename (configuration for 'log' command, firmware file for DFU)", 
                                               false, "", "filename");
    cmd.add(inFilenameArg);

    // --output filename
    // Only required for log operation.
    TCLAP::ValueArg<std::string> outFilenameArg("o", "output", 
                                                "Output filename (sensor .dsf log for 'log' command, logger .json configuration for 'template' command)", 
                                                false, "", "filename");
    cmd.add(outFilenameArg);

#ifdef _WIN32    
    // --device device_number
    TCLAP::ValueArg<unsigned> deviceArg("d", "device", "FTDI device number (0 if only one FTDI UART<->USB adapter is connected.)",
                                           false, 1, "device-num");
    cmd.add(deviceArg);
#else
    // --device device_name
    TCLAP::ValueArg<std::string> deviceArg("d", "device", 
                                           "Serial port device name",
                                           false, "", "device-name");
    cmd.add(deviceArg);
#endif

    // --clear-dcd
    // clears DCD at start of session.
    TCLAP::SwitchArg clearDcdArg("", "clear-dcd", "Clear dynamic IMU calibration at logger start", false);
    cmd.add(clearDcdArg);
    
    // --clear-of-cal
    // clears optical flow cal at start of session.
    TCLAP::SwitchArg clearOfCalArg("", "clear-of-cal", "Clear optical flow calibration at logger start", false);
    cmd.add(clearOfCalArg);

    // Parse them arguments
    cmd.parse(argc, argv);

    // Store values in member variables
    m_cmd = cmdArg.getValue();
    m_outFilenameSet = outFilenameArg.isSet();
    m_outFilename = outFilenameArg.getValue();
    m_inFilenameSet = inFilenameArg.isSet();
    m_inFilename = inFilenameArg.getValue();
    m_deviceArgSet = deviceArg.isSet();
    m_deviceArg = deviceArg.getValue();
    m_clearDcd = clearDcdArg.getValue();
    m_clearOfCal = clearOfCalArg.getValue();
}

int Sh2Logger::run() {
    if (m_cmd == "template") {
        return do_template();
    }

    else if (m_cmd == "dfu-bno") {
        return do_dfu_bno();
    }

    else if (m_cmd == "dfu-fsp200") {
        return do_dfu_fsp();
    }

    else if (m_cmd == "log") {
        return do_logging();
    }

    std::cout << "ERROR: Unrecognized command: " << m_cmd << std::endl;
    return -1;
}

int Sh2Logger::do_template() {
    // JSON configuration file template
    static const json templateContents = {
        { "calEnable", "0x00" },
        { "clearDcd", false },
        { "clearOfCal", false },
        { "dcdAutoSave", false },
        { "orientation", "ned" },
        { "rawSampleTime", false},
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
                { "IZRO Motion Request", 0 },                   // 0x2B
                { "Raw Optical Flow", {
                        {"rate", 0},
                        {"sensorSpecific",0}}},                     // 0x2C
                { "Dead Reckoning Pose", 0},                    // 0x2D
                { "Wheel Encoder", 0},                          // 0x2E
            } //sensorList value 
        }, //sensorList
    };

    if (!m_outFilenameSet) {
        std::cout << "ERROR: No output file specified, use -o or --output argument." << std::endl;
        return -1;
    }

    // Write a configuration file template
    std::cout << "\nGenerate a configuration file template \"" << m_outFilename << "\". \n";
    std::ofstream ofile(m_outFilename);
    ofile << std::setw(4) << templateContents << std::endl;

    return 0;
}

int Sh2Logger::do_logging() {

    // Start logging
    if (!m_inFilenameSet) {
        std::cout << "ERROR: No config file specified, use -i or --input argument." << std::endl;
        return -1;
    }
    if (!m_outFilenameSet) {
        std::cout << "ERROR: No output file specified, use -o or --output argument." << std::endl;
        return -1;
    }
    if (!m_deviceArgSet) {
        std::cout << "ERROR: No device specified, use -d or --device argument." << std::endl;
        return -1;
    }

    ConsoleLogger consoleLogger;
    Logger* logger;
    LoggerApp loggerApp;
    LoggerApp::appConfig_s appConfig;
    DsfLogger dsfLogger;
    
    // Parse input .json file
    if (!ParseJsonBatchFile(m_inFilename, &appConfig)) {
        // Error in .json file
        printf("Error in .json file\n");
        return -1;
    }

    // Override certain params from the command line
    if (m_clearDcd) {
        appConfig.clearDcd = true;
    }
    if (m_clearOfCal) {
        appConfig.clearOfCal = true;
    }


    // --------------------------------------------------------------------------------------------
	// Start Application
	// --------------------------------------------------------------------------------------------
	runApp_ = true;

    if (appConfig.pSensorsToEnable->empty()) {
        std::cout << "ERROR: No sensors enabled. Abort.\n";
        return -1;
    }

	// Initialize DSF Logger
    bool rv = dsfLogger.init(m_outFilename.c_str(), appConfig.orientationNed);
    if (rv) {
        logger = &dsfLogger;
    } else {
        std::cout << "ERROR: Unable to open dsf file:  \"" << m_outFilename << "\"" << std::endl;
        std::cout << "ERROR: Display sensor data on the console instead." << std::endl;

        // Issue with the specified DSF output file path.
        // Use the ConsoleLogger to show sensor data on the console instead.
        consoleLogger.init(m_outFilename.c_str(), appConfig.orientationNed);
        logger = &consoleLogger;
    }

    // Initialze FTDI HAL
    int status;
#ifdef _WIN32
    /// status = ftdiHal_.init(m_deviceArg, &timer_);
    sh2_Hal_t *pHal = ftdi_hal_init(m_deviceArg);
#else
    /// status = ftdiHal_.init(m_deviceArg.c_str(), &timer_);
    sh2_Hal_t *pHal = ftdi_hal_init(m_deviceArg.c_str());
#endif

    if (pHal == 0) {
        std::cout << "ERROR: Initialize FTDI HAL failed!\n";
        return -1;
    }
    /// FtdiHal* pFtdiHal = &ftdiHal_;

    // Initialize the LoggerApp
    status = loggerApp.init(&appConfig, pHal, logger);
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

	uint32_t currSysTime_us = pHal->getTimeUs(pHal);
	uint32_t lastChecked_us = currSysTime_us;

    while (runApp_) {

#ifdef _WIN32
        currSysTime_us = pHal->getTimeUs(pHal);
		
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

        loggerApp.service();
    }

    std::cout << "\nINFO: Shutting down" << std::endl;

    loggerApp.finish();

#ifdef _WIN32
    SetConsoleMode(hstdin, mode);
#endif
    return 0;
}

int Sh2Logger::do_dfu_bno() {
    // Make sure a filename was specified
    if (!m_inFilenameSet) {
        std::cout << "ERROR: No firmware file specified, use -i or --input argument." << std::endl;
        return -1;
    }
    
    // Perform DFU for BNO
    // Create a HAL for BNO08x DFU
    if (!m_deviceArgSet) {
        std::cout << "ERROR: No serial device specified, use --device argument." << std::endl;
        return -1;
    }
    
#ifdef _WIN32
    sh2_Hal_t *pHal = bno_dfu_hal_init(m_deviceArg);
#else
    sh2_Hal_t *pHal = bno_dfu_hal_init(m_deviceArg.c_str());
#endif
    
    if (pHal == 0) {
        fprintf(stderr, "Error initializing DFU HAL.\n");
        return -1;
    }

    // Open firmware file
    Firmware *firmware = new HcBinFile(m_inFilename);
        
    // BNO08x DFU
    BnoDfu bnoDfu;
    printf("Starting DFU for BNO08x.\n");
    if (!bnoDfu.run(pHal, firmware)) {
        // DFU Failed.
        std::cout << "ERROR: DFU for BNO08x failed.\n";
        delete firmware;
        return -1;
    }
    
    printf("DFU completed successfully.\n");
    delete firmware;
    return 0;
}

int Sh2Logger::do_dfu_fsp() {
    // Make sure a filename was specified
    if (!m_inFilenameSet) {
        std::cout << "ERROR: No firmware file specified, use -i or --input argument." << std::endl;
        return -1;
    }
    
    // Perform DFU for BNO
    // Create a HAL for BNO08x DFU
    if (!m_deviceArgSet) {
        std::cout << "ERROR: No serial device specified, use --device argument." << std::endl;
        return -1;
    }

#ifdef _WIN32
    sh2_Hal_t *pHal = ftdi_hal_dfu_init(m_deviceArg);
#else
    sh2_Hal_t *pHal = ftdi_hal_dfu_init(m_deviceArg.c_str());
#endif
    
    if (pHal == 0) {
        fprintf(stderr, "Error initializing DFU HAL.\n");
        return -1;
    }
        
    // FSP200 DFU
    FspDfu fspDfu;
    Firmware * firmware = new HcBinFile(m_inFilename);
    printf("Starting DFU for FSP200.\n");
    if (!fspDfu.run(pHal, firmware)) {
        // DFU Failed.
        std::cout << "ERROR: DFU for FSP200 failed.\n";
        return -1;
    }
    
    printf("DFU completed successfully.\n");
    return 0;
}

// -----------------------------------------------------------------------  

// List of sensors to be enabled
static LoggerApp::sensorList_t sensorsToEnable_;

// =================================================================================================
// LOCAL FUNCTIONS
// =================================================================================================
#ifndef _WIN32
void breakHandler(int signo) {
    if (signo == SIGINT) {
        if (!runApp_) {
            fprintf(stderr, "force quit\n");
            exit(0);
        }
        runApp_ = false;
    }
}
#endif

// ================================================================================================
// MAIN
// ================================================================================================
 int main(int argc, const char* argv[]) {

#ifndef _WIN32
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = breakHandler;
    sigaction(SIGINT, &act, NULL);
#endif

    Sh2Logger sh2_logger;

    // Process command line, setting up sh2_logger to run its operation
    sh2_logger.parseArgs(argc, argv);

    // Run the operation
    return sh2_logger.run();
}


// ================================================================================================
// ParseJsonBatchFile
// ================================================================================================
bool ParseJsonBatchFile(std::string inFilename, LoggerApp::appConfig_s* pAppConfig) {
    json jBat;
    bool foundSensorList = false;

    std::cout << "\nINFO: (json) Process the batch json file \'" << inFilename  << "\' ... \n";

    // Batch file is specified, extract the configuration options.
    std::ifstream ifile(inFilename);

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
        } else if (it.key().compare("clearOfCal") == 0) {
            pAppConfig->clearOfCal = it.value();
            std::cout << "INFO: (json) Clear OF Cal : ";
            if (pAppConfig->clearOfCal) {
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

        } else if (it.key().compare("rawSampleTime") == 0) {
            pAppConfig->useRawSampleTime = it.value();
            std::cout << "INFO: (json) Use Raw Sensor Sample Time : ";
            if (pAppConfig->useRawSampleTime) {
                std::cout << "Enable\n";
            } else {
                std::cout << "Disable\n";
            }
        } else if (it.key().compare("sensorList") == 0) {
            foundSensorList = true;

            std::cout << "INFO: (json) Extract Sensor list ... \n";

            for (json::iterator sl = it.value().begin(); sl != it.value().end(); ++sl) {
                LoggerApp::SensorFeatureSet_s config;
                config.sensorId = (sh2_SensorId_t)LoggerUtil::findSensorIdByName(sl.key().c_str());
                float rate = 0;
                if(sl.value().is_number()) { 
                    rate = sl.value(); 
                } else if (sl.value().is_object()){
                    for (json::iterator sc = sl.value().begin(); sc != sl.value().end(); ++sc){
                        if (strcmp(sc.key().c_str(), "rate") == 0){
                            rate = sc.value();
                        } else if (strcmp(sc.key().c_str(), "sensorSpecific") == 0){
                            config.sensorSpecific = sc.value();
                        }
                        //TODO: add sniffEnabled
                    }
                }
                config.reportInterval_us = 0;
                if (rate > 0) {
                    config.reportInterval_us = static_cast<uint32_t>((1e6 / rate) + 0.5);
                }

                // Add the new sensor to sensorsToEnable_ if the ID is valid.
                if (config.sensorId <= SH2_MAX_SENSOR_ID && config.reportInterval_us > 0) {
                    std::cout << "INFO: (json)      Sensor ID : " << static_cast<uint32_t>(config.sensorId);
                    std::cout << " - " << sl.key();
                    std::cout << " @ " << (1e6 / config.reportInterval_us) << "Hz";
                    std::cout << " (" << config.reportInterval_us << "us)";
                    std::cout << " [ss="<< config.sensorSpecific << "]\n";

                    sensorsToEnable_.push_back(config);
                }
            }

            sensorsToEnable_.sort();
            sensorsToEnable_.unique();

            pAppConfig->pSensorsToEnable = &sensorsToEnable_;
        }
    }

    if (!foundSensorList) {
        std::cout << "\nERROR: \"sensorList\" is not specified in the json file. Abort. \n";
        return false;
    }

    std::cout << "\n";
    return true;
}
