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

#pragma once

extern "C" {
#include "sh2.h"
#include "sh2_SensorValue.h"
}

#include <chrono>

/**
 * A WheelSource is responsible for reporting wheel encoder position
 * data to an sh2 instance via calls to sh2_reportWheelEncoder.
 *
 * It is responsible for maintaining a mapping between the local host
 * time and the recipient's internal time.
 *
 * This base class provides a simple mechanism for obtaining this
 * mapping (between "raw" sensor data and a local clock provided by
 * std::chrono::steady_clock).
 *
 * Implementations should override the `service` method.
 *
 * Care should be taken to avoid blocking as much as possible.
 */
class WheelSource {
public:
    WheelSource();

    /**
     * Report a sensor sample, which WheelSource may use to establish
     * the local/recipient timestamp mapping.
     *
     * The default implementation assumes no skew between host and
     * module clock.
     */
    void reportModuleTime(const sh2_SensorValue_t* value, const sh2_SensorEvent_t* event);

    /**
     * Check for new wheel data: this method is responsible for
     * calling sh2_reportWheelEncoder if data is available.
     */
    virtual void service(void) = 0;

protected:
    /**
     * Obtain an estimate of the module (hub) time for a given local
     * (host) time_point.
     */
    uint32_t estimateHubTime(const std::chrono::steady_clock::time_point* t = nullptr);

    /**
     * Return true if sufficient data has been read from the module to
     * obtain a mapping betwen module and host time.
     */
    bool ready(void);

private:
    bool ready_;
    uint32_t lastHub_;
    std::chrono::steady_clock::time_point lastHost_;
};
