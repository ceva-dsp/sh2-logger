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

#ifndef LOGGER_UTIL_H
#define LOGGER_UTIL_H

extern "C" {
#include "sh2.h"
}
#include <stdint.h>

// =================================================================================================
// DEFINES AND MACROS
// =================================================================================================


// =================================================================================================
// DATA TYPES
// =================================================================================================

// =================================================================================================
// LoggerUtil
// =================================================================================================
namespace LoggerUtil {
    // ---------------------------------------------------------------------------------------------
    // DATA TYPE
    // ---------------------------------------------------------------------------------------------
    struct frsString_s {
        uint16_t recordId;
        char const* name;
    };

    struct sensorSpec_s {
        char const* name;
        const sh2_SensorConfig_t* config;
    };

    // ---------------------------------------------------------------------------------------------
    // CONSTANTS
    // ---------------------------------------------------------------------------------------------
    // Sensors which requires special configuration
    static const sh2_SensorConfig_t DefaultConfigSpec = { false, false, false, false, 0, 0, 0, 0 };

    static const sh2_SensorConfig_t PacConfigSpec = { false, false, false, false, 0, 0, 0, 511 };
    static const sh2_SensorConfig_t StabilityClassifierConfigSpec = { true, true, false, false, 0, 0, 0, 0 };
    static const sh2_SensorConfig_t StepDetectorConfigSpec = { true, false, false, false, 0, 0, 0, 0 };
    static const sh2_SensorConfig_t StepCounterConfigSpec = { true, false, false, false, 0, 0, 0, 0 };
    static const sh2_SensorConfig_t PressureConfigSpec = { false, false, false, false, 0, 0, 0, 0 };
    static const sh2_SensorConfig_t AmbientLightConfigSpec = { true, true, false, false, (uint16_t)(0.1 * (2 ^ 6)), 0, 0, 0 };
    static const sh2_SensorConfig_t TemperatureConfigSpec = { true, true, false, false, (uint16_t)(0.1 * (2 ^ 7)), 0, 0, 0 };
    static const sh2_SensorConfig_t HumidityConfigSpec = { true, true, false, false, (uint16_t)(0.1 * (2 ^ 8)), 0, 0, 0 };
    static const sh2_SensorConfig_t ProximityConfigSpec = { true, true, true, true, (uint16_t)(1.0 * (2 ^ 4)), 0, 0, 0 };
    static const sh2_SensorConfig_t TapDetectorConfigSpec = { true, false, false, false, 0, 0, 0, 0 };
    static const sh2_SensorConfig_t ShakeDetectorConfigSpec = { true, false, true, true, 0, 0, 0, 0 };
    static const sh2_SensorConfig_t FlipDetectorConfigSpec = { true, false, true, true, 0, 0, 0, 0 };
    static const sh2_SensorConfig_t StabilityDetectorConfigSpec = { true, false, false, false, 0, 0, 0, 0 };
    static const sh2_SensorConfig_t SleepDetectorConfigSpec = { true, true, true, true, 0, 0, 0, 0 };
    static const sh2_SensorConfig_t TiltDetectorConfigSpec = { true, false, true, true, 0, 0, 0, 0 };
    static const sh2_SensorConfig_t PocketDetectorConfigSpec = { true, false, true, true, 0, 0, 0, 0 };
    static const sh2_SensorConfig_t CircleDetectorConfigSpec = { true, false, true, true, 0, 0, 0, 0 };


    static const sensorSpec_s SensorSpec[] = {
        { "Reserved", &DefaultConfigSpec },                             // 0x00
        { "Accelerometer", &DefaultConfigSpec },                        // 0x01
        { "Gyroscope", &DefaultConfigSpec },                            // 0x02
        { "Magnetic Field", &DefaultConfigSpec },                       // 0x03
        { "Linear Acceleration", &DefaultConfigSpec },                  // 0x04
        { "Rotation Vector", &DefaultConfigSpec },                      // 0x05
        { "Gravity", &DefaultConfigSpec },                              // 0x06
        { "Uncalibrated Gyroscope", &DefaultConfigSpec },               // 0x07
        { "Game Rotation Vector", &DefaultConfigSpec },                 // 0x08
        { "Geomagnetic Rotation Vector", &DefaultConfigSpec },          // 0x09
        { "Pressure", &PressureConfigSpec },                            // 0x0A
        { "Ambient Light", &AmbientLightConfigSpec },                   // 0x0B
        { "Humidity", &HumidityConfigSpec },                            // 0x0C
        { "Proximity", &ProximityConfigSpec },                          // 0x0D
        { "Temperature", &TemperatureConfigSpec },                      // 0x0E
        { "Uncalibrated MagneticField", &DefaultConfigSpec },           // 0x0F
        { "Tap Detector", &TapDetectorConfigSpec },                     // 0x10
        { "Step Counter", &StepCounterConfigSpec },                     // 0x11
        { "Significant Motion", &DefaultConfigSpec },                   // 0x12
        { "Stability Classifier", &StabilityClassifierConfigSpec },     // 0x13
        { "Raw Accelerometer", &DefaultConfigSpec },                    // 0x14
        { "Raw Gyroscope", &DefaultConfigSpec },                        // 0x15
        { "Raw Magnetometer", &DefaultConfigSpec },                     // 0x16
        { "Reserved", &DefaultConfigSpec },                             // 0x17
        { "Step Detector", &StepDetectorConfigSpec },                   // 0x18
        { "Shake Detector", &ShakeDetectorConfigSpec },                 // 0x19
        { "Flip Detector", &FlipDetectorConfigSpec },                   // 0x1A
        { "Pickup Detector", &DefaultConfigSpec },                      // 0x1B
        { "Stability Detector", &StabilityDetectorConfigSpec },         // 0x1C
        { "Reserved", &DefaultConfigSpec },                             // 0x1D
        { "Personal Activity Classifier", &PacConfigSpec },             // 0x1E
        { "Sleep Detector", &SleepDetectorConfigSpec },                 // 0x1F
        { "Tilt Detector", &TiltDetectorConfigSpec },                   // 0x20
        { "Pocket Detector", &PocketDetectorConfigSpec },               // 0x21
        { "Circle Detector", &CircleDetectorConfigSpec },               // 0x22
        { "Heart Rate Monitor", &DefaultConfigSpec },                   // 0x23
        { "Reserved", &DefaultConfigSpec },                             // 0x24
        { "Reserved", &DefaultConfigSpec },                             // 0x25
        { "Reserved", &DefaultConfigSpec },                             // 0x26
        { "Reserved", &DefaultConfigSpec },                             // 0x27
        { "ARVR Stabilized Rotation Vector", &DefaultConfigSpec },      // 0x28
        { "ARVR Stabilized GameRotation Vector", &DefaultConfigSpec },  // 0x29
        { "Gyro Rotation Vector", &DefaultConfigSpec },                 // 0x2A
        { "IZRO Motion Request", &DefaultConfigSpec },                  // 0x2B
    };
    static_assert((sizeof(SensorSpec) / sizeof(sensorSpec_s)) == (SH2_MAX_SENSOR_ID + 1),
        "Const variable size match failed");

    static const frsString_s Bno080FrsRecords[] = {
        // { STATIC_CALIBRATION_AGM, "scd" },
        { NOMINAL_CALIBRATION, "nominal_scd" },
        { DYNAMIC_CALIBRATION, "dcd" },
        { STATIC_CALIBRATION_SRA, "sra_scd" },
        { NOMINAL_CALIBRATION_SRA, "sra_nominal_scd" },

        { ME_POWER_MGMT, "motion_engine_power_management" },
        { SYSTEM_ORIENTATION, "system_orientation" },
        { ACCEL_ORIENTATION, "accelerometer_orientation" },
        { SCREEN_ACCEL_ORIENTATION, "sra_accelerometer_orientation" },
        { GYROSCOPE_ORIENTATION, "gyroscope_orientation" },
        { MAGNETOMETER_ORIENTATION, "magnetometer_orientation" },
        { ARVR_STABILIZATION_RV, "arvr_rotation_vector" },
        { ARVR_STABILIZATION_GRV, "arvr_game_rotation_vector" },

        // {TAP_DETECT_CONFIG, "tap_detector_configuration"},
        { SIG_MOTION_DETECT_CONFIG, "significant_motion_detector_configuration" },
        { SHAKE_DETECT_CONFIG, "shake_detector_configuration" },
        { MAX_FUSION_PERIOD, "maximum_fusion_period" },
        { SERIAL_NUMBER, "serial_number" },
        { ES_PRESSURE_CAL, "pressure_calibration" },
        { ES_TEMPERATURE_CAL, "temperature_calibration" },
        { ES_HUMIDITY_CAL, "humidity_calibration" },
        { ES_AMBIENT_LIGHT_CAL, "ambient_light_calibration" },
        { ES_PROXIMITY_CAL, "proximity_calibration" },
        { ALS_CAL, "ambient_light_special_calibration" },
        { PROXIMITY_SENSOR_CAL, "proximity_special_calibration" },
        { PICKUP_DETECTOR_CONFIG, "pickup_detector_configuration" },
        { FLIP_DETECTOR_CONFIG, "flip_detector_configuration" },
        { STABILITY_DETECTOR_CONFIG, "stability_detector_configuration" },
        { ACTIVITY_TRACKER_CONFIG, "activity_tracker_configuration" },
        { SLEEP_DETECTOR_CONFIG, "sleep_detector_configuration" },
        { TILT_DETECTOR_CONFIG, "tilt_detector_configuration" },
        { POCKET_DETECTOR_CONFIG, "pocket_detector_configuration" },
        { CIRCLE_DETECTOR_CONFIG, "circle_detector_configuration" },
        { USER_RECORD, "user_record" },
        { ME_TIME_SOURCE_SELECT, "motion_engine_time_source_selection" },
        { UART_FORMAT, "uart_output_format_selection" },
        { GYRO_INTEGRATED_RV_CONFIG, "gyro_integrated_rotation_vector_configuration" },
    };

    static const uint8_t NumBno080FrsRecords = sizeof(Bno080FrsRecords) / sizeof(frsString_s);


    // ---------------------------------------------------------------------------------------------
    // HELPER FUNCTIONS
    // ---------------------------------------------------------------------------------------------
    static int findSensorIdByName(char const * name) {

        for (int i = 0; i <= SH2_MAX_SENSOR_ID; i++) {
            if (strcmp(name, SensorSpec[i].name) == 0) {
                return i;
            }
        }
        // Return an invalid Sensor ID if no sensor is matched.
        return SH2_MAX_SENSOR_ID + 1;
    }

};

#endif // LOGGER_UTIL_H
