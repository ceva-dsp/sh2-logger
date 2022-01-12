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

#include "ConsoleLogger.h"
#include <iomanip>
#include <iostream>


// =================================================================================================
// DEFINES AND MACROS
// =================================================================================================


// =================================================================================================
// DATA TYPES
// =================================================================================================


// =================================================================================================
// LOCAL VARIABLES
// =================================================================================================


// =================================================================================================
// PUBLIC FUNCTIONS
// =================================================================================================
// -------------------------------------------------------------------------------------------------
// ConsoleLogger::init
// -------------------------------------------------------------------------------------------------
bool ConsoleLogger::init(char const* filePath, bool ned) {
    orientationNed_ = ned;
    return true;
}

// -------------------------------------------------------------------------------------------------
// ConsoleLogger::finish
// -------------------------------------------------------------------------------------------------
void ConsoleLogger::finish() {
}

// -------------------------------------------------------------------------------------------------
// ConsoleLogger::logMessage
// -------------------------------------------------------------------------------------------------
void ConsoleLogger::logMessage(char const* msg) {
    std::cout << msg << std::endl;
}

// -------------------------------------------------------------------------------------------------
// ConsoleLogger::logAsyncEvent
// -------------------------------------------------------------------------------------------------
void ConsoleLogger::logAsyncEvent(sh2_AsyncEvent_t* pEvent, double currTime) {

    switch (pEvent->eventId) {
        case SH2_RESET:
            break;
        case SH2_GET_FEATURE_RESP:
            break;
        default:
            break;
    }
}

// -------------------------------------------------------------------------------------------------
// ConsoleLogger::logProductIds
// -------------------------------------------------------------------------------------------------
void ConsoleLogger::logProductIds(sh2_ProductIds_t ids) {
    for (uint32_t i = 0; i < ids.numEntries; ++i) {
        switch (ids.entry[i].resetCause) {
            default:
            case 0:
                break;
            case 1:
                std::cout << "!RESET_CAUSE=\"PowerOnReset\"\n";
                break;
            case 2:
                std::cout << "!RESET_CAUSE=\"InternalSystemReset\"\n";
                break;
            case 3:
                std::cout << "!RESET_CAUSE=\"WatchdogTimeout\"\n";
                break;
            case 4:
                std::cout << "!RESET_CAUSE=\"ExternalReset\"\n";
                break;
            case 5:
                std::cout << "!RESET_CAUSE=\"Other\"\n";
                break;
        }
        std::cout << "! PN." << i << "=\"";
        std::cout << static_cast<uint32_t>(ids.entry[i].swPartNumber) << " ";
        std::cout << static_cast<uint32_t>(ids.entry[i].swVersionMajor) << ".";
        std::cout << static_cast<uint32_t>(ids.entry[i].swVersionMinor) << ".";
        std::cout << static_cast<uint32_t>(ids.entry[i].swVersionPatch) << ".";
        std::cout << static_cast<uint32_t>(ids.entry[i].swBuildNumber) << "\"\n";
    }
}

// -------------------------------------------------------------------------------------------------
// ConsoleLogger::logFrsRecord
// -------------------------------------------------------------------------------------------------
void ConsoleLogger::logFrsRecord(char const* name, uint32_t* buffer, uint16_t words) {
}

// -------------------------------------------------------------------------------------------------
// ConsoleLogger::logSensorValue
// -------------------------------------------------------------------------------------------------
void ConsoleLogger::logSensorValue(sh2_SensorValue_t* pValue, double currTime, int64_t delay_uS) {
    uint32_t id = pValue->sensorId;

    switch (id) {

        case SH2_RAW_ACCELEROMETER: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.rawAccelerometer.x << ",";
            std::cout << pValue->un.rawAccelerometer.y << ",";
            std::cout << pValue->un.rawAccelerometer.z << "\n";
            break;
        }
        case SH2_ACCELEROMETER: {
            WriteSensorReportHeader(pValue, currTime);
            if (orientationNed_) {
                std::cout << pValue->un.accelerometer.y << ","; // ENU -> NED
                std::cout << pValue->un.accelerometer.x << ",";
                std::cout << -pValue->un.accelerometer.z << ",";
            }
            else {
                std::cout << pValue->un.accelerometer.x << ",";
                std::cout << pValue->un.accelerometer.y << ",";
                std::cout << pValue->un.accelerometer.z << ",";
            }
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_LINEAR_ACCELERATION: {
            WriteSensorReportHeader(pValue, currTime);
            if (orientationNed_) {
                std::cout << pValue->un.linearAcceleration.y << ","; // ENU -> NED
                std::cout << pValue->un.linearAcceleration.x << ",";
                std::cout << -pValue->un.linearAcceleration.z << ",";
            }
            else {
                std::cout << pValue->un.linearAcceleration.x << ",";
                std::cout << pValue->un.linearAcceleration.y << ",";
                std::cout << pValue->un.linearAcceleration.z << ",";
            }
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_GRAVITY: {
            WriteSensorReportHeader(pValue, currTime);
            if (orientationNed_) {
                std::cout << pValue->un.gravity.y << ","; // ENU -> NED
                std::cout << pValue->un.gravity.x << ",";
                std::cout << -pValue->un.gravity.z << ",";
            }
            else {
                std::cout << pValue->un.gravity.x << ",";
                std::cout << pValue->un.gravity.y << ",";
                std::cout << pValue->un.gravity.z << ",";
            }
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_RAW_GYROSCOPE: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.rawGyroscope.x << ",";
            std::cout << pValue->un.rawGyroscope.y << ",";
            std::cout << pValue->un.rawGyroscope.z << ",";
            std::cout << pValue->un.rawGyroscope.temperature << "\n";
            break;
        }
        case SH2_GYROSCOPE_CALIBRATED: {
            WriteSensorReportHeader(pValue, currTime);
            if (orientationNed_) {
                std::cout << pValue->un.gyroscope.y << ","; // ENU -> NED
                std::cout << pValue->un.gyroscope.x << ",";
                std::cout << -pValue->un.gyroscope.z << ",";
            }
            else {
                std::cout << pValue->un.gyroscope.x << ",";
                std::cout << pValue->un.gyroscope.y << ",";
                std::cout << pValue->un.gyroscope.z << ",";
            }
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_GYROSCOPE_UNCALIBRATED: {
            WriteSensorReportHeader(pValue, currTime);
            if (orientationNed_) {
                std::cout << pValue->un.gyroscopeUncal.y << ","; // ENU -> NED
                std::cout << pValue->un.gyroscopeUncal.x << ",";
                std::cout << -pValue->un.gyroscopeUncal.z << ",";
                std::cout << pValue->un.gyroscopeUncal.biasY << ","; // ENU -> NED
                std::cout << pValue->un.gyroscopeUncal.biasX << ",";
                std::cout << -pValue->un.gyroscopeUncal.biasZ;
            }
            else {
                std::cout << pValue->un.gyroscopeUncal.x << ",";
                std::cout << pValue->un.gyroscopeUncal.y << ",";
                std::cout << pValue->un.gyroscopeUncal.z << ",";
                std::cout << pValue->un.gyroscopeUncal.biasX << ",";
                std::cout << pValue->un.gyroscopeUncal.biasY << ",";
                std::cout << pValue->un.gyroscopeUncal.biasZ << ",";
            }
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_RAW_MAGNETOMETER: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.rawMagnetometer.x << ",";
            std::cout << pValue->un.rawMagnetometer.y << ",";
            std::cout << pValue->un.rawMagnetometer.z << "\n";
            break;
        }
        case SH2_MAGNETIC_FIELD_CALIBRATED: {
            WriteSensorReportHeader(pValue, currTime);
            if (orientationNed_) {
                std::cout << pValue->un.magneticField.y << ","; // ENU -> NED
                std::cout << pValue->un.magneticField.x << ",";
                std::cout << -pValue->un.magneticField.z << ",";
            }
            else {
                std::cout << pValue->un.magneticField.x << ",";
                std::cout << pValue->un.magneticField.y << ",";
                std::cout << pValue->un.magneticField.z << ",";
            }
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_MAGNETIC_FIELD_UNCALIBRATED: {
            WriteSensorReportHeader(pValue, currTime);
            if (orientationNed_) {
                std::cout << pValue->un.magneticFieldUncal.y << ","; // ENU -> NED
                std::cout << pValue->un.magneticFieldUncal.x << ",";
                std::cout << -pValue->un.magneticFieldUncal.z << ",";
                std::cout << pValue->un.magneticFieldUncal.biasY << ","; // ENU -> NED
                std::cout << pValue->un.magneticFieldUncal.biasX << ",";
                std::cout << -pValue->un.magneticFieldUncal.biasZ << ",";
            }
            else {
                std::cout << pValue->un.magneticFieldUncal.x << ",";
                std::cout << pValue->un.magneticFieldUncal.y << ",";
                std::cout << pValue->un.magneticFieldUncal.z << ",";
                std::cout << pValue->un.magneticFieldUncal.biasX << ",";
                std::cout << pValue->un.magneticFieldUncal.biasY << ",";
                std::cout << pValue->un.magneticFieldUncal.biasZ << ",";
            }
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_ROTATION_VECTOR: {
            WriteSensorReportHeader(pValue, currTime);
            if (orientationNed_) {
                std::cout << pValue->un.rotationVector.real << ",";
                std::cout << pValue->un.rotationVector.j << ","; // Convert ENU -> NED
                std::cout << pValue->un.rotationVector.i << ",";
                std::cout << -pValue->un.rotationVector.k << ",";
            }
            else {
                std::cout << pValue->un.rotationVector.real << ",";
                std::cout << pValue->un.rotationVector.i << ",";
                std::cout << pValue->un.rotationVector.j << ",";
                std::cout << pValue->un.rotationVector.k << ",";
            }
            std::cout << RadiansToDeg(pValue->un.rotationVector.accuracy) << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_GAME_ROTATION_VECTOR: {
            WriteSensorReportHeader(pValue, currTime);
            if (orientationNed_) {
                std::cout << pValue->un.gameRotationVector.real << ",";
                std::cout << pValue->un.gameRotationVector.j << ","; // Convert ENU -> NED
                std::cout << pValue->un.gameRotationVector.i << ",";
                std::cout << -pValue->un.gameRotationVector.k;
            }
            else {
                std::cout << pValue->un.gameRotationVector.real << ",";
                std::cout << pValue->un.gameRotationVector.i << ",";
                std::cout << pValue->un.gameRotationVector.j << ",";
                std::cout << pValue->un.gameRotationVector.k;
            }
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_GEOMAGNETIC_ROTATION_VECTOR: {
            WriteSensorReportHeader(pValue, currTime);
            if (orientationNed_) {
                std::cout << pValue->un.geoMagRotationVector.real << ",";
                std::cout << pValue->un.geoMagRotationVector.j << ","; // Convert ENU -> NED
                std::cout << pValue->un.geoMagRotationVector.i << ",";
                std::cout << -pValue->un.geoMagRotationVector.k << ",";
            }
            else {
                std::cout << pValue->un.geoMagRotationVector.real << ",";
                std::cout << pValue->un.geoMagRotationVector.i << ",";
                std::cout << pValue->un.geoMagRotationVector.j << ",";
                std::cout << pValue->un.geoMagRotationVector.k << ",";
            }
            std::cout << RadiansToDeg(pValue->un.geoMagRotationVector.accuracy) << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_PRESSURE: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.pressure.value << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_AMBIENT_LIGHT: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.ambientLight.value << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_HUMIDITY: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.humidity.value << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_PROXIMITY: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.proximity.value << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_TEMPERATURE: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.temperature.value << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_TAP_DETECTOR: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << static_cast<uint32_t>(pValue->un.tapDetector.flags) << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_STEP_DETECTOR: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.stepDetector.latency << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_STEP_COUNTER: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.stepCounter.steps << ",";
            std::cout << pValue->un.stepCounter.latency << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_SIGNIFICANT_MOTION: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.sigMotion.motion << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_STABILITY_CLASSIFIER: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << static_cast<uint32_t>(pValue->un.stabilityClassifier.classification) << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_SHAKE_DETECTOR: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.shakeDetector.shake << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_FLIP_DETECTOR: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.flipDetector.flip << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_PICKUP_DETECTOR: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.pickupDetector.pickup << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_STABILITY_DETECTOR: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.stabilityDetector.stability << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_PERSONAL_ACTIVITY_CLASSIFIER: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << static_cast<uint32_t>(pValue->un.personalActivityClassifier.mostLikelyState) << ",";
            for (int i = 0; i < 10; i++) {
                std::cout << static_cast<uint32_t>(pValue->un.personalActivityClassifier.confidence[i]) << ",";
            }
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_SLEEP_DETECTOR: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << static_cast<uint32_t>(pValue->un.sleepDetector.sleepState) << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_TILT_DETECTOR: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.tiltDetector.tilt << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_POCKET_DETECTOR: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.pocketDetector.pocket << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_CIRCLE_DETECTOR: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.circleDetector.circle << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_HEART_RATE_MONITOR: {
            WriteSensorReportHeader(pValue, currTime);
            std::cout << pValue->un.heartRateMonitor.heartRate << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_ARVR_STABILIZED_RV: {
            WriteSensorReportHeader(pValue, currTime);
            if (orientationNed_) {
                std::cout << pValue->un.arvrStabilizedRV.real << ",";
                std::cout << pValue->un.arvrStabilizedRV.j << ","; // Convert ENU -> NED
                std::cout << pValue->un.arvrStabilizedRV.i << ",";
                std::cout << -pValue->un.arvrStabilizedRV.k << ",";
            }
            else {
                std::cout << pValue->un.arvrStabilizedRV.real << ",";
                std::cout << pValue->un.arvrStabilizedRV.i << ",";
                std::cout << pValue->un.arvrStabilizedRV.j << ",";
                std::cout << pValue->un.arvrStabilizedRV.k << ",";
            }
            std::cout << RadiansToDeg(pValue->un.arvrStabilizedRV.accuracy) << ",";
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_ARVR_STABILIZED_GRV: {
            WriteSensorReportHeader(pValue, currTime);
            if (orientationNed_) {
                std::cout << pValue->un.arvrStabilizedGRV.real << ",";
                std::cout << pValue->un.arvrStabilizedGRV.j << ","; // Convert ENU -> NED
                std::cout << pValue->un.arvrStabilizedGRV.i << ",";
                std::cout << -pValue->un.arvrStabilizedGRV.k << ",";
            }
            else {
                std::cout << pValue->un.arvrStabilizedGRV.real << ",";
                std::cout << pValue->un.arvrStabilizedGRV.i << ",";
                std::cout << pValue->un.arvrStabilizedGRV.j << ",";
                std::cout << pValue->un.arvrStabilizedGRV.k << ",";
            }
            std::cout << static_cast<uint32_t>(pValue->status) << "\n";
            break;
        }
        case SH2_GYRO_INTEGRATED_RV: {
            WriteSensorReportHeader(pValue, currTime);
            if (orientationNed_) {
                std::cout << pValue->un.gyroIntegratedRV.real << ",";
                std::cout << pValue->un.gyroIntegratedRV.j << ","; // Convert ENU -> NED
                std::cout << pValue->un.gyroIntegratedRV.i << ",";
                std::cout << -pValue->un.gyroIntegratedRV.k << ",";
                std::cout << pValue->un.gyroIntegratedRV.angVelY << ",";
                std::cout << pValue->un.gyroIntegratedRV.angVelX << ",";
                std::cout << -pValue->un.gyroIntegratedRV.angVelZ;
            } else {
                std::cout << pValue->un.gyroIntegratedRV.real << ",";
                std::cout << pValue->un.gyroIntegratedRV.i << ",";
                std::cout << pValue->un.gyroIntegratedRV.j << ",";
                std::cout << pValue->un.gyroIntegratedRV.k << ",";
                std::cout << pValue->un.gyroIntegratedRV.angVelX << ",";
                std::cout << pValue->un.gyroIntegratedRV.angVelY << ",";
                std::cout << pValue->un.gyroIntegratedRV.angVelZ;
            }
            std::cout << "\n";
            break;
        }
        default:
            break;
    }
}


// =================================================================================================
// PRIVATE FUNCTIONS
// =================================================================================================
void ConsoleLogger::WriteSensorReportHeader(sh2_SensorValue_t* pValue, double timestamp) {
    std::cout << "." << static_cast<uint32_t>(pValue->sensorId) << " ";
    std::cout << std::fixed << std::setprecision(9) << timestamp << ",";
    std::cout.unsetf(std::ios_base::floatfield);
}
