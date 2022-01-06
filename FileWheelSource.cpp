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

#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "FileWheelSource.h"
#include <cstring>
#include <iostream>
#include <thread>


FileWheelSource::FileWheelSource(const char* fn)
    : lastWheelSN_(-1), hubTime_(0), input_(&inputFile_), running_(true) {
    // Open file, or assign to std::cin.
    if (0 == strcmp(fn, "-")) {
        input_ = &(std::cin);
    } else {
        std::cout << "opening " << fn << "..." << std::flush;
        inputFile_.open(fn, std::ifstream::in);
        std::cout << "...done" << std::endl;
    }

    reader_ = std::thread{[&] {
        std::string tmp;
        while (running_) {
            std::getline(*input_, tmp);
            if (tmp != "") {
                std::lock_guard<std::mutex> lock(mtx_);
                q_.push_back(std::move(tmp));
            }
            if (input_->eof()) {
                // At EOF, clear status and sleep briefly so as not to
                // peg CPU
                input_->clear();
                std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(1));
            }
        }
    }};
}

FileWheelSource::~FileWheelSource(void) {
    if (inputFile_.is_open()) {
        inputFile_.close();
    }
    running_ = false;
    reader_.join();
}

void FileWheelSource::service(void) {
    std::string line("");
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!q_.empty()) {
            line = q_.front();
            q_.pop_front();
        }
    }
    if (line != "") {
        int wheelIndex = -1;
        int wheelData = -1;
        int dataType = -1;
        int wheelSN = -1;
        if (4 ==
            sscanf(line.c_str(), "%d,%d,%d,%d", &wheelSN, &wheelIndex, &wheelData, &dataType)) {
            if (ready()) {
                // If this is a new wheel SN, get current hub time
                // estimate. Otherwise, use the last hub time
                // estimate: this is how the host indicates that two
                // wheel measurements were simultaneous.
                if (wheelSN != lastWheelSN_) {
                    hubTime_ = estimateHubTime();
                    lastWheelSN_ = wheelSN;
                }
                sh2_reportWheelEncoder(wheelIndex, hubTime_, wheelData, dataType);
            }
        } else {
            std::cerr << "Discard bad wheel report line:" << line << std::endl;
        }
    }
}
