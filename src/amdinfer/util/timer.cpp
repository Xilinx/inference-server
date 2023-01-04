// Copyright 2022 Advanced Micro Devices, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file
 * @brief Implements a helper timer class
 */

#include "amdinfer/util/timer.hpp"

namespace amdinfer::util {

TimePoint getTime() { return std::chrono::high_resolution_clock::now(); }

Timer::Timer(TimePoint time) { add("start", time); }

Timer::Timer(bool start) {
  if (start) {
    this->start();
  }
}

void Timer::start() { add("start"); }

void Timer::stop() { add("stop"); }

void Timer::add(const std::string& label) { add(label, getTime()); }

void Timer::add(const std::string& label, TimePoint time) {
  auto point = times_.find(label);
  if (point == times_.end()) {
    times_.try_emplace(label, time);
  } else {
    point->second = time;
  }
}

void Timer::clear() { times_.clear(); }

}  // namespace amdinfer::util
