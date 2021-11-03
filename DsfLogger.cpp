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

#include "DsfLogger.h"
#include <iomanip>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#include <stdint.h>
#else
#include <time.h>
#endif


// =================================================================================================
// DEFINES AND MACROS
// =================================================================================================


// =================================================================================================
// DATA TYPES
// =================================================================================================
struct sensorDsfHeader_s {
    char const* name;
    char const* sensorColumns;
};


// =================================================================================================
// LOCAL VARIABLES
// =================================================================================================
static const sensorDsfHeader_s SensorDsfHeader[] = {
    { "Reserved", "" },                                                                               // 0x00
    { "Accelerometer", "LIN_ACC_GRAVITY[xyz]{m/s^2}" },                                               // 0x01
    { "Gyroscope", "ANG_VEL[xyz]{rad/s}" },                                                           // 0x02
    { "MagneticField", "MAG[xyz]{m/s^2}" },                                                           // 0x03
    { "LinearAcceleration", "LIN_ACC[xyz]{m/s^2}" },                                                  // 0x04
    { "RotationVector", "ANG_POS_GLOBAL[wxyz]{quaternion},ANG_POS_ACCURACY[x]{deg}" },                // 0x05
    { "Gravity", "GRAVITY[xyz]{m/s^2}" },                                                             // 0x06
    { "UncalibratedGyroscope", "ANG_VEL[xyz]{rad/s},BIAS[xyz]{rad/s}" },                              // 0x07
    { "GameRotationVector", "ANG_POS_GLOBAL[wxyz]{quaternion}" },                                     // 0x08
    { "GeomagneticRotationVector", "ANG_POS_GLOBAL[wxyz]{quaternion},ANG_POS_ACCURACY[x]{deg}" },     // 0x09
    { "Pressure", "PRESSURE[x]{hPa}" },                                                               // 0x0A
    { "AmbientLight", "AMBIENT_LIGHT[x]{lux}" },                                                      // 0x0B
    { "Humidity", "HUMIDITY[x]{%}" },                                                                 // 0x0C
    { "Proximity", "PROXIMITY[x]{cm}" },                                                              // 0x0D
    { "Temperature", "TEMPERATURE[x]{degC}" },                                                        // 0x0E
    { "UncalibratedMagField", "MAG_UNCAL[xyz]{m/s^2},MAG_BAIS[xyz]{m/s^2}" },                         // 0x0F
    { "TapDetector", "TAP_DETECTOR[x]{state}" },                                                      // 0x10
    { "StepCounter", "STEPS[x]{steps},STEP_COUNTER_LATENCY[x]{us}" },                                 // 0x11
    { "SignificantMotion", "SIGNIFICANT_MOTION[x]{state}" },                                          // 0x12
    { "StabilityClassifier", "STABILITY_CLASSIFIER[x]{state}" },                                      // 0x13
    { "RawAccelerometer", "LIN_ACC_GRAVITY[xyz]{ADC},SAMPLE_TIME[x]{us}" },                           // 0x14
    { "RawGyroscope", "ANG_VEL[xyz]{ADC},TEMPERATURE[x]{ADC},SAMPLE_TIME[x]{us}" },                   // 0x15
    { "RawMagnetometer", "MAG[xyz]{ADC},SAMPLE_TIME[x]{us}" },                                        // 0x16
    { "Reserved", "" },                                                                               // 0x17
    { "StepDetector", "STEP_DETECTOR_LATENCY[x]{us}" },                                               // 0x18
    { "ShakeDetector", "SHAKE_DETECTOR[x]{state}" },                                                  // 0x19
    { "FlipDetector", "FLIP_DETECTOR[x]{state}" },                                                    // 0x1A
    { "PickupDetector", "PICKUP_DETECTOR[x]{state}" },                                                // 0x1B
    { "StabilityDetector", "STABILITY_DETECTOR[x]{state}" },                                          // 0x1C
    { "Reserved", "" },                                                                               // 0x1D
    { "PersonalActivityClassifier", "MOST_LIKELY_STATE[x]{state},CONFIDENCE[uvbfstwrax]{state}" },    // 0x1E
    { "SleepDetector", "SLEEP_DETECTOR[x]{state}" },                                                  // 0x1F
    { "TiltDetector", "TILT_DETECTOR[x]{state}" },                                                    // 0x20
    { "PocketDetector", "POCKET_DETECTOR[x]{state}" },                                                // 0x21
    { "CircleDetector", "CIRCLE_DETECTOR[x]{state}" },                                                // 0x22
    { "HeartRateMonitor", "HEART_RATE_MONITOR[x]{?}" },                                               // 0x23
    { "Reserved", "" },                                                                               // 0x24
    { "Reserved", "" },                                                                               // 0x25
    { "Reserved", "" },                                                                               // 0x26
    { "Reserved", "" },                                                                               // 0x27
    { "ARVRStabilizedRotationVector", "ANG_POS_GLOBAL[wxyz]{quaternion},ANG_POS_ACCURACY[x]{deg}" },  // 0x28
    { "ARVRStabilizedGameRotationVector", "ANG_POS_GLOBAL[wxyz]{quaternion}" },                       // 0x29
    { "GyroIntegratedRV", "ANG_POS_GLOBAL[wxyz]{quaternion},ANG_VEL[xyz]{rad/s}" },                   // 0x2A
    { "MotionRequest", "MOTION_INTENT[x]{state},MOTION_REQUEST[x]{state}" },                          // 0x2B
    { "RawOpticalFlow",
    "MOVED{bool},LASER_ON{bool},LIN_VEL_XY[xy]{ADC},SQUAL,RES[xy],SHUTTER,FRAME_MAX,FRAME_AVG,FRAME_MIN,DT{us},SAMPLE_TIME[x]{us}" },   // 0x2C
    { "DeadReckoningPose", "LIN_POS_GLOBAL[xyz]{m},ANG_POS_GLOBAL[wxyz]{quaternion},LIN_VEL[xyz]{m/s},ANG_VEL[xyz]{rad/s},SAMPLE_TIME[x]{us}" }, // 0x2D
    { "WheelEncoder", "TIME{s},SAMPLE_ID[x],DATA_TYPE[x],WHEEL_INDEX[x],DATA[x],TIMESTAMP{us}" }, // 0x2E
};

static_assert((sizeof(SensorDsfHeader) / sizeof(sensorDsfHeader_s)) == (SH2_MAX_SENSOR_ID + 1),
    "Const variable size match failed");

static SampleIdExtender * extenders_[SH2_MAX_SENSOR_ID + 1];


// =================================================================================================
// PUBLIC FUNCTIONS
// =================================================================================================
// -------------------------------------------------------------------------------------------------
// DsfLogger::init
// -------------------------------------------------------------------------------------------------
bool DsfLogger::init(char const* filePath, bool ned) {
    outFile_.open(filePath);

    if (outFile_) {
        orientationNed_ = ned;

        for (int i = 0; i <= SH2_MAX_SENSOR_ID; i++) {
            if (strcmp("", SensorDsfHeader[i].sensorColumns) == 0) {
                extenders_[i] = nullptr;
            }
            else {
                extenders_[i] = new SampleIdExtender();
            }
        }
        return true;

    } else {
        return false;
    }
}

// -------------------------------------------------------------------------------------------------
// DsfLogger::finish
// -------------------------------------------------------------------------------------------------
void DsfLogger::finish() {
    if (outFile_) {
        outFile_.flush();
        outFile_.close();
    }
}

// -------------------------------------------------------------------------------------------------
// DsfLogger::logMessage
// -------------------------------------------------------------------------------------------------
void DsfLogger::logMessage(char const* msg) {
    outFile_ << msg << std::endl;
}

// -------------------------------------------------------------------------------------------------
// DsfLogger::logAsyncEvent
// -------------------------------------------------------------------------------------------------
void DsfLogger::logAsyncEvent(sh2_AsyncEvent_t* pEvent, double timestamp) {

    switch (pEvent->eventId) {
        case SH2_RESET:
            outFile_ << "$ ";
            outFile_ << std::fixed << std::setprecision(9) << timestamp << ",";
            outFile_.unsetf(std::ios_base::floatfield);
            outFile_ << " reset(1)\n";
            break;
        case SH2_GET_FEATURE_RESP: {
            // Log the sensor reporting interval
            sh2_SensorId_t sensorId = pEvent->sh2SensorConfigResp.sensorId;
            // Ensure the Channel definition is written before the period is reported.
            if (extenders_[sensorId]) {
                if (extenders_[sensorId]->isEmpty()) {
                    WriteChannelDefinition(sensorId);
                    extenders_[sensorId]->extend(0);
                }
                outFile_ << "$" << static_cast<int32_t>(sensorId) << " ";
                outFile_ << std::fixed << std::setprecision(9) << timestamp << ", ";
                outFile_.unsetf(std::ios_base::floatfield);
                outFile_ << "period(";
                outFile_ << (pEvent->sh2SensorConfigResp.sensorConfig.reportInterval_us / 1000000.0);
                outFile_ << ")\n";
            }
            break;
        }
        default:
            break;
    }
}

// -------------------------------------------------------------------------------------------------
// DsfLogger::logProductIds
// -------------------------------------------------------------------------------------------------
void DsfLogger::logProductIds(sh2_ProductIds_t ids) {
    for (uint32_t i = 0; i < ids.numEntries; ++i) {
        switch (ids.entry[i].resetCause) {
            default:
            case 0:
                break;
            case 1:
                outFile_ << "!RESET_CAUSE=\"PowerOnReset\"\n";
                break;
            case 2:
                outFile_ << "!RESET_CAUSE=\"InternalSystemReset\"\n";
                break;
            case 3:
                outFile_ << "!RESET_CAUSE=\"WatchdogTimeout\"\n";
                break;
            case 4:
                outFile_ << "!RESET_CAUSE=\"ExternalReset\"\n";
                break;
            case 5:
                outFile_ << "!RESET_CAUSE=\"Other\"\n";
                break;
        }
        outFile_ << "! PN." << i << "=\""
            << static_cast<uint32_t>(ids.entry[i].swPartNumber) << " "
            << static_cast<uint32_t>(ids.entry[i].swVersionMajor) << "."
            << static_cast<uint32_t>(ids.entry[i].swVersionMinor) << "."
            << static_cast<uint32_t>(ids.entry[i].swVersionPatch) << "."
            << static_cast<uint32_t>(ids.entry[i].swBuildNumber) << "\"\n";
    }
}

// -------------------------------------------------------------------------------------------------
// DsfLogger::logFrsRecord
// -------------------------------------------------------------------------------------------------
void DsfLogger::logFrsRecord(char const* name, uint32_t* buffer, uint16_t words) {
    outFile_ << "!" << name << "=\"";
    for (uint16_t w = 0; w < words; ++w) {
        for (uint8_t b = 0; b < 4; ++b) {
            outFile_ << std::hex << std::setw(2) << std::setfill('0')
                     << ((buffer[w] >> (b * 8)) & 0xFF);
            if (w != (words - 1) || b != 3) {
                outFile_ << ",";
            }
        }
    }
    outFile_ << std::dec << "\"\n";
}

// -------------------------------------------------------------------------------------------------
// DsfLogger::logSensorValue
// -------------------------------------------------------------------------------------------------
void DsfLogger::logSensorValue(sh2_SensorValue_t* pValue, double timestamp) {
    uint32_t sensorId = pValue->sensorId;

    if (!extenders_[sensorId]) {
        // If the sensor ID is invalid, return.
        return;
    }

    // Write Sensor Report Header
    WriteSensorReportHeader(pValue, extenders_[sensorId], timestamp);

    switch (sensorId) {

        case SH2_RAW_ACCELEROMETER: {
            outFile_ << pValue->un.rawAccelerometer.x << ","
                << pValue->un.rawAccelerometer.y << ","
                << pValue->un.rawAccelerometer.z << ","
                << pValue->un.rawAccelerometer.timestamp << "\n";
            break;
        }
        case SH2_ACCELEROMETER: {
            if (orientationNed_) {
                outFile_ << pValue->un.accelerometer.y << "," // ENU -> NED
                    << pValue->un.accelerometer.x << ","
                    << -pValue->un.accelerometer.z << "\n";
            } else {
                outFile_ << pValue->un.accelerometer.x << ","
                    << pValue->un.accelerometer.y << ","
                    << pValue->un.accelerometer.z << "\n";
            }
            break;
        }
        case SH2_LINEAR_ACCELERATION: {
            if (orientationNed_) {
                outFile_ << pValue->un.linearAcceleration.y << "," // ENU -> NED
                    << pValue->un.linearAcceleration.x << ","
                    << -pValue->un.linearAcceleration.z << "\n";
            } else {
                outFile_ << pValue->un.linearAcceleration.x << ","
                    << pValue->un.linearAcceleration.y << ","
                    << pValue->un.linearAcceleration.z << "\n";
            }
            break;
        }
        case SH2_GRAVITY: {
            if (orientationNed_) {
                outFile_ << pValue->un.gravity.y << "," // ENU -> NED
                    << pValue->un.gravity.x << ","
                    << -pValue->un.gravity.z << "\n";
            } else {
                outFile_ << pValue->un.gravity.x << ","
                    << pValue->un.gravity.y << ","
                    << pValue->un.gravity.z << "\n";
            }
            break;
        }
        case SH2_RAW_GYROSCOPE: {
            outFile_ << pValue->un.rawGyroscope.x << ","
                << pValue->un.rawGyroscope.y << ","
                << pValue->un.rawGyroscope.z << ","
                << pValue->un.rawGyroscope.temperature << ","
                << pValue->un.rawGyroscope.timestamp << "\n";
            break;
        }
        case SH2_GYROSCOPE_CALIBRATED: {
            if (orientationNed_) {
                outFile_ << pValue->un.gyroscope.y << "," // ENU -> NED
                    << pValue->un.gyroscope.x << ","
                    << -pValue->un.gyroscope.z << "\n";
            } else {
                outFile_ << pValue->un.gyroscope.x << ","
                    << pValue->un.gyroscope.y << ","
                    << pValue->un.gyroscope.z << "\n";
            }
            break;
        }
        case SH2_GYROSCOPE_UNCALIBRATED: {
            if (orientationNed_) {
                outFile_ << pValue->un.gyroscopeUncal.y << "," // ENU -> NED
                    << pValue->un.gyroscopeUncal.x << ","
                    << -pValue->un.gyroscopeUncal.z << ","
                    << pValue->un.gyroscopeUncal.biasY << "," // ENU -> NED
                    << pValue->un.gyroscopeUncal.biasX << ","
                    << -pValue->un.gyroscopeUncal.biasZ << "\n";
            } else {
                outFile_ << pValue->un.gyroscopeUncal.x << ","
                    << pValue->un.gyroscopeUncal.y << ","
                    << pValue->un.gyroscopeUncal.z << ","
                    << pValue->un.gyroscopeUncal.biasX << ","
                    << pValue->un.gyroscopeUncal.biasY << ","
                    << pValue->un.gyroscopeUncal.biasZ << "\n";
            }
            break;
        }
        case SH2_RAW_MAGNETOMETER: {
            outFile_ << pValue->un.rawMagnetometer.x << ","
                << pValue->un.rawMagnetometer.y << ","
                << pValue->un.rawMagnetometer.z << ","
                << pValue->un.rawMagnetometer.timestamp << "\n";
            break;
        }
        case SH2_MAGNETIC_FIELD_CALIBRATED: {
            if (orientationNed_) {
                outFile_ << pValue->un.magneticField.y << "," // ENU -> NED
                    << pValue->un.magneticField.x << ","
                    << -pValue->un.magneticField.z << "\n";
            } else {
                outFile_ << pValue->un.magneticField.x << ","
                    << pValue->un.magneticField.y << ","
                    << pValue->un.magneticField.z << "\n";
            }
            break;
        }
        case SH2_MAGNETIC_FIELD_UNCALIBRATED: {
            if (orientationNed_) {
                outFile_ << pValue->un.magneticFieldUncal.y << "," // ENU -> NED
                    << pValue->un.magneticFieldUncal.x << ","
                    << -pValue->un.magneticFieldUncal.z << ","
                    << pValue->un.magneticFieldUncal.biasY << "," // ENU -> NED
                    << pValue->un.magneticFieldUncal.biasX << ","
                    << -pValue->un.magneticFieldUncal.biasZ << "\n";
            } else {
                outFile_ << pValue->un.magneticFieldUncal.x << ","
                    << pValue->un.magneticFieldUncal.y << ","
                    << pValue->un.magneticFieldUncal.z << ","
                    << pValue->un.magneticFieldUncal.biasX << ","
                    << pValue->un.magneticFieldUncal.biasY << ","
                    << pValue->un.magneticFieldUncal.biasZ << "\n";
            }
            break;
        }
        case SH2_ROTATION_VECTOR: {
            if (orientationNed_) {
                outFile_ << pValue->un.rotationVector.real << ","
                    << pValue->un.rotationVector.j << "," // Convert ENU -> NED
                    << pValue->un.rotationVector.i << ","
                    << -pValue->un.rotationVector.k << ",";
            } else {
                outFile_ << pValue->un.rotationVector.real << ","
                    << pValue->un.rotationVector.i << ","
                    << pValue->un.rotationVector.j << ","
                    << pValue->un.rotationVector.k << ",";
            }
            outFile_ << RadiansToDeg(pValue->un.rotationVector.accuracy) << "\n";
            break;
        }
        case SH2_GAME_ROTATION_VECTOR: {
            if (orientationNed_) {
                outFile_ << pValue->un.gameRotationVector.real << ","
                    << pValue->un.gameRotationVector.j << "," // Convert ENU -> NED
                    << pValue->un.gameRotationVector.i << ","
                    << -pValue->un.gameRotationVector.k << "\n";
            } else {
                outFile_ << pValue->un.gameRotationVector.real << ","
                    << pValue->un.gameRotationVector.i << ","
                    << pValue->un.gameRotationVector.j << ","
                    << pValue->un.gameRotationVector.k << "\n";
            }
            break;
        }
        case SH2_GEOMAGNETIC_ROTATION_VECTOR: {
            if (orientationNed_) {
                outFile_ << pValue->un.geoMagRotationVector.real << ","
                    << pValue->un.geoMagRotationVector.j << "," // Convert ENU -> NED
                    << pValue->un.geoMagRotationVector.i << ","
                    << -pValue->un.geoMagRotationVector.k << ",";
            } else {
                outFile_ << pValue->un.geoMagRotationVector.real << ","
                    << pValue->un.geoMagRotationVector.i << ","
                    << pValue->un.geoMagRotationVector.j << ","
                    << pValue->un.geoMagRotationVector.k << ",";
            }
            outFile_ << RadiansToDeg(pValue->un.geoMagRotationVector.accuracy) << "\n";
            break;
        }
        case SH2_PRESSURE: {
            outFile_ << pValue->un.pressure.value << "\n";
            break;
        }
        case SH2_AMBIENT_LIGHT: {
            outFile_ << pValue->un.ambientLight.value << "\n";
            break;
        }
        case SH2_HUMIDITY: {
            outFile_ << pValue->un.humidity.value << "\n";
            break;
        }
        case SH2_PROXIMITY: {
            outFile_ << pValue->un.proximity.value << "\n";
            break;
        }
        case SH2_TEMPERATURE: {
            outFile_ << pValue->un.temperature.value << "\n";
            break;
        }
        case SH2_TAP_DETECTOR: {
            outFile_ << static_cast<uint32_t>(pValue->un.tapDetector.flags) << "\n";
            break;
        }
        case SH2_STEP_DETECTOR: {
            outFile_ << pValue->un.stepDetector.latency << "\n";
            break;
        }
        case SH2_STEP_COUNTER: {
            outFile_ << pValue->un.stepCounter.steps << ",";
            outFile_ << pValue->un.stepCounter.latency << "\n";
            break;
        }
        case SH2_SIGNIFICANT_MOTION: {
            outFile_ << pValue->un.sigMotion.motion << "\n";
            break;
        }
        case SH2_STABILITY_CLASSIFIER: {
            outFile_ << static_cast<uint32_t>(pValue->un.stabilityClassifier.classification) << "\n";
            break;
        }
        case SH2_SHAKE_DETECTOR: {
            outFile_ << pValue->un.shakeDetector.shake << "\n";
            break;
        }
        case SH2_FLIP_DETECTOR: {
            outFile_ << pValue->un.flipDetector.flip << "\n";
            break;
        }
        case SH2_PICKUP_DETECTOR: {
            outFile_ << pValue->un.pickupDetector.pickup << "\n";
            break;
        }
        case SH2_STABILITY_DETECTOR: {
            outFile_ << pValue->un.stabilityDetector.stability << "\n";
            break;
        }
        case SH2_PERSONAL_ACTIVITY_CLASSIFIER: {
            outFile_ << static_cast<uint32_t>(pValue->un.personalActivityClassifier.mostLikelyState) << ",";
            for (int i = 0; i < 10; i++) {
                outFile_ << static_cast<uint32_t>(pValue->un.personalActivityClassifier.confidence[i]) << ",";
            }
            outFile_ << "\n";
            break;
        }
        case SH2_SLEEP_DETECTOR: {
            outFile_ << static_cast<uint32_t>(pValue->un.sleepDetector.sleepState) << "\n";
            break;
        }
        case SH2_TILT_DETECTOR: {
            outFile_ << pValue->un.tiltDetector.tilt << "\n";
            break;
        }
        case SH2_POCKET_DETECTOR: {
            outFile_ << pValue->un.pocketDetector.pocket << "\n";
            break;
        }
        case SH2_CIRCLE_DETECTOR: {
            outFile_ << pValue->un.circleDetector.circle << "\n";
            break;
        }
        case SH2_HEART_RATE_MONITOR: {
            outFile_ << pValue->un.heartRateMonitor.heartRate << "\n";
            break;
        }
        case SH2_ARVR_STABILIZED_RV: {
            if (orientationNed_) {
                outFile_ << pValue->un.arvrStabilizedRV.real << ","
                    << pValue->un.arvrStabilizedRV.j << "," // Convert ENU -> NED
                    << pValue->un.arvrStabilizedRV.i << ","
                    << -pValue->un.arvrStabilizedRV.k << ",";
            } else {
                outFile_ << pValue->un.arvrStabilizedRV.real << ","
                    << pValue->un.arvrStabilizedRV.i << ","
                    << pValue->un.arvrStabilizedRV.j << ","
                    << pValue->un.arvrStabilizedRV.k << ",";
            }
            outFile_ << RadiansToDeg(pValue->un.arvrStabilizedRV.accuracy) << "\n";
            break;
        }
        case SH2_ARVR_STABILIZED_GRV: {
            if (orientationNed_) {
                outFile_ << pValue->un.arvrStabilizedGRV.real << ","
                    << pValue->un.arvrStabilizedGRV.j << "," // Convert ENU -> NED
                    << pValue->un.arvrStabilizedGRV.i << ","
                    << -pValue->un.arvrStabilizedGRV.k << "\n";
            } else {
                outFile_ << pValue->un.arvrStabilizedGRV.real << ","
                    << pValue->un.arvrStabilizedGRV.i << ","
                    << pValue->un.arvrStabilizedGRV.j << ","
                    << pValue->un.arvrStabilizedGRV.k << "\n";
            }
            break;
        }
        case SH2_GYRO_INTEGRATED_RV: {
            if (orientationNed_) {
                outFile_ << pValue->un.gyroIntegratedRV.real << ","
                    << pValue->un.gyroIntegratedRV.j << "," // Convert ENU -> NED
                    << pValue->un.gyroIntegratedRV.i << ","
                    << -pValue->un.gyroIntegratedRV.k << ","
                    << pValue->un.gyroIntegratedRV.angVelY << ","
                    << pValue->un.gyroIntegratedRV.angVelX << ","
                    << -pValue->un.gyroIntegratedRV.angVelZ << "\n";
            } else {
                outFile_ << pValue->un.gyroIntegratedRV.real << ","
                    << pValue->un.gyroIntegratedRV.i << ","
                    << pValue->un.gyroIntegratedRV.j << ","
                    << pValue->un.gyroIntegratedRV.k << ","
                    << pValue->un.gyroIntegratedRV.angVelX << ","
                    << pValue->un.gyroIntegratedRV.angVelY << ","
                    << pValue->un.gyroIntegratedRV.angVelZ << "\n";
            }
            break;
        }
        case SH2_IZRO_MOTION_REQUEST: {
            outFile_ << static_cast<uint32_t>(pValue->un.izroRequest.intent) << ","
                << static_cast<uint32_t>(pValue->un.izroRequest.request) << "\n";
            break;
        }
        case SH2_RAW_OPTICAL_FLOW:{
            outFile_ << static_cast<uint32_t>(pValue->un.rawOptFlow.dx != 0 || pValue->un.rawOptFlow.dy != 0) << ",";
            outFile_ << static_cast<uint32_t>(pValue->un.rawOptFlow.laserOn) << ",";
            outFile_ << static_cast<int16_t>(pValue->un.rawOptFlow.dx) << ",";
            outFile_ << static_cast<int16_t>(pValue->un.rawOptFlow.dy) << ",";
            outFile_ << static_cast<uint32_t>(pValue->un.rawOptFlow.iq) << ",";
            outFile_ << static_cast<uint32_t>(pValue->un.rawOptFlow.resX) << ",";
            outFile_ << static_cast<uint32_t>(pValue->un.rawOptFlow.resY) << ",";
            outFile_ << static_cast<uint32_t>(pValue->un.rawOptFlow.shutter) << ",";
            outFile_ << static_cast<uint32_t>(pValue->un.rawOptFlow.frameMax) << ",";
            outFile_ << static_cast<uint32_t>(pValue->un.rawOptFlow.frameAvg) << ",";
            outFile_ << static_cast<uint32_t>(pValue->un.rawOptFlow.frameMin) << ",";
            outFile_ << static_cast<uint32_t>(pValue->un.rawOptFlow.dt) << ",";
            outFile_ << static_cast<uint32_t>(pValue->un.rawOptFlow.timestamp) << "\n";
            break;
        }
        case SH2_DEAD_RECKONING_POSE:{
            // Output is ENU: rearrange if desired
            if (orientationNed_){
                outFile_ << pValue->un.deadReckoningPose.linPosY << ",";
                outFile_ << pValue->un.deadReckoningPose.linPosX << ",";
                outFile_ << -pValue->un.deadReckoningPose.linPosZ << ",";

                outFile_ << pValue->un.deadReckoningPose.real << ",";
                outFile_ << pValue->un.deadReckoningPose.j << ",";
                outFile_ << pValue->un.deadReckoningPose.i << ",";
                outFile_ << -pValue->un.deadReckoningPose.k << ",";

                outFile_ << pValue->un.deadReckoningPose.linVelY << ",";
                outFile_ << pValue->un.deadReckoningPose.linVelX << ",";
                outFile_ << -pValue->un.deadReckoningPose.linVelZ << ",";

                outFile_ << pValue->un.deadReckoningPose.angVelY << ",";
                outFile_ << pValue->un.deadReckoningPose.angVelX << ",";
                outFile_ << -pValue->un.deadReckoningPose.angVelZ << ",";
            } else {
                outFile_ << pValue->un.deadReckoningPose.linPosX << ",";
                outFile_ << pValue->un.deadReckoningPose.linPosY << ",";
                outFile_ << pValue->un.deadReckoningPose.linPosZ << ",";

                outFile_ << pValue->un.deadReckoningPose.real << ",";
                outFile_ << pValue->un.deadReckoningPose.i << ",";
                outFile_ << pValue->un.deadReckoningPose.j << ",";
                outFile_ << pValue->un.deadReckoningPose.k << ",";

                outFile_ << pValue->un.deadReckoningPose.linVelX << ",";
                outFile_ << pValue->un.deadReckoningPose.linVelY << ",";
                outFile_ << pValue->un.deadReckoningPose.linVelZ << ",";

                outFile_ << pValue->un.deadReckoningPose.angVelX << ",";
                outFile_ << pValue->un.deadReckoningPose.angVelY << ",";
                outFile_ << pValue->un.deadReckoningPose.angVelZ << ",";
            }
            outFile_ << pValue->un.deadReckoningPose.timestamp << "\n";
            break;
        }
        default:
            break;
    }
}


// =================================================================================================
// PRIVATE FUNCTIONS
// =================================================================================================
// -------------------------------------------------------------------------------------------------
// DsfLogger::WriteChannelDefinition
// -------------------------------------------------------------------------------------------------
void DsfLogger::WriteChannelDefinition(uint8_t sensorId, bool orientation) {
    char const* fieldNames = SensorDsfHeader[sensorId].sensorColumns;
    char const* name = SensorDsfHeader[sensorId].name;

    outFile_ << "+" << static_cast<int32_t>(sensorId) 
        << " TIME{s},SYSTEM_TIME{s},SAMPLE_ID[x]{samples},STATUS[x]{state}," 
        << fieldNames << "\n";

    if (orientation) {
        if (sensorId != SH2_RAW_ACCELEROMETER && sensorId != SH2_RAW_GYROSCOPE &&
            sensorId != SH2_RAW_MAGNETOMETER) {
            outFile_ << "!" << static_cast<int32_t>(sensorId) << " coordinate_system=";
            if (orientationNed_) {
                outFile_ << "\"NED\"\n";
            }
            else {
                outFile_ << "\"ENU\"\n";
            }
        }
    }
    outFile_ << "!" << static_cast<int32_t>(sensorId) << " name=\"" << name << "\"\n";
}

// -------------------------------------------------------------------------------------------------
// DsfLogger::WriteSensorReportHeader
// -------------------------------------------------------------------------------------------------
void DsfLogger::WriteSensorReportHeader(sh2_SensorValue_t* pValue, SampleIdExtender* extender, double timestamp) {
    if (extender->isEmpty()) {
        WriteChannelDefinition(pValue->sensorId);
    }

    if (posixOffset_ == 0 && timestamp != 0){
#ifdef _WIN32
        const DWORD64 UNIX_EPOCH = 0x019DB1DED53E8000; // January 1, 1970 in Windows Ticks.
        const double TICKS_PER_SECOND = 10000000.0;    // Windows tick = 100ns.

        // Get system time
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);

        // Combine parts into LARGE_INTEGER ticks.
        LARGE_INTEGER ticks;
        ticks.LowPart = ft.dwLowDateTime;
        ticks.HighPart = ft.dwHighDateTime;

        // Convert to double, based on Unix epoch, units are seconds.
        double posix_time = (ticks.QuadPart - UNIX_EPOCH) / TICKS_PER_SECOND;

        // Store offset to convert timestamps to Unix times.
        posixOffset_ = posix_time - timestamp;
#else
        struct timespec tp;
        clock_gettime(CLOCK_REALTIME, &tp);
        double posix_time = tp.tv_sec + tp.tv_nsec*1e-9;
        posixOffset_ = posix_time - timestamp;
#endif
    }

    outFile_ << "." << static_cast<uint32_t>(pValue->sensorId) << " ";
    outFile_ << std::fixed << std::setprecision(9) << timestamp << ",";
    outFile_ << std::fixed << std::setprecision(9) << (timestamp + posixOffset_) << ",";
    outFile_.unsetf(std::ios_base::floatfield);
    outFile_ << extender->extend(pValue->sequence) << ",";
    outFile_ << static_cast<uint32_t>(pValue->status) << ",";
}
