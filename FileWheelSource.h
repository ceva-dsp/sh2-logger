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

#pragma once
#include "WheelSource.h"
#include <deque>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>

/**
 * Implementation of WheelSource that attempts to read data from an
 * input file and reports via SH2 API when new measurements are
 * available.
 *
 * Data format is:
 * <sequence_number>,<wheel_index>,<value>,<data_type>\n
 *
 * Where:
 * - sequence_number: used to indicate when pairs of samples
 *   correspond to the same point in time.
 * - wheel_index: 0 = left wheel, 1 = right wheel
 * - value: The wheel encoder position or velocity. This is a
 *   16 bit unsigned integer (0-65536).
 * - data_type: 0 = wheel position, 1 = wheel velocity.
 */
class FileWheelSource : public WheelSource {
public:
    /**
     * Create a new thread and start attempting to read wheel data
     * from file.
     *
     * If fn is "-", read from std::cin.
     *
     * Note that the file is opened in the calling thread (not the
     * reader thread), so care should be taken not to block if wheel
     * data is not expected to be available at startup. e.g. if fn is
     * a named pipe, the "open" operation will not complete until the
     * other end is connected.
     */
    FileWheelSource(const char* fn);
    ~FileWheelSource();

    virtual void service(void);

private:
    // Last wheel sequence number read from file
    int lastWheelSN_;
    // hub time associated with last sample
    uint32_t hubTime_;

    // Input data source, assigned to either inputFile_ or std::cin
    std::istream* input_;
    // Input file stream (if not std::cin)
    std::ifstream inputFile_;

    // Queue for read-but-not-parsed data lines
    std::deque<std::string> q_;
    // Mutex used for locking q_
    std::mutex mtx_;
    // Thread responsible for reading input from file
    std::thread reader_;
    // Flag used for cleanup/join
    bool running_;
};
