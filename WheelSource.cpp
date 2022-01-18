/*
 * Copyright 2018-2022 CEVA, Inc.
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

#include "WheelSource.h"
using namespace std::chrono;

#include <iostream>

WheelSource::WheelSource() : lastHub_(0), ready_(false) {
    lastHost_ = steady_clock::now();
}

void WheelSource::reportModuleTime(const sh2_SensorValue_t* value, const sh2_SensorEvent_t* event) {
    // Basic mapping between module and host time. Does not attempt to
    // correct for clock skew, and does not attempt to compute
    // e.g. a moving average of offset between host and module time.
    switch (value->sensorId) {
        case SH2_RAW_ACCELEROMETER:
            ready_ = true;
            lastHub_ = value->un.rawAccelerometer.timestamp;
            lastHost_ = steady_clock::now();
            break;
        case SH2_RAW_GYROSCOPE:
            ready_ = true;
            lastHub_ = value->un.rawGyroscope.timestamp;
            lastHost_ = steady_clock::now();
            break;
        case SH2_RAW_MAGNETOMETER:
            ready_ = true;
            lastHub_ = value->un.rawMagnetometer.timestamp;
            lastHost_ = steady_clock::now();
            break;
        case SH2_RAW_OPTICAL_FLOW:
            ready_ = true;
            lastHub_ = value->un.rawOptFlow.timestamp;
            lastHost_ = steady_clock::now();
            break;
        default:
            // No raw timestamp to update
            break;
    }
}

bool WheelSource::ready(void) {
    return ready_;
}

uint32_t WheelSource::estimateHubTime(const steady_clock::time_point* t) {
    steady_clock::time_point now;
    if (t == nullptr) {
        now = steady_clock::now();
        t = &now;
    }
    duration<double> elapsed = duration_cast<duration<double> >(*t - lastHost_);

    int32_t elapsed_us = static_cast<int32_t>(elapsed.count() * 1e6);
    return lastHub_ + elapsed_us;
}
