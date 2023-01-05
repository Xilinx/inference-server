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
 * @brief Defines a helper timer class
 */

#ifndef GUARD_AMDINFER_UTIL_TIMER
#define GUARD_AMDINFER_UTIL_TIMER

#include <chrono>         // IWYU pragma: export
#include <ratio>          // for ratio
#include <string>         // for string
#include <unordered_map>  // for unordered_map

namespace amdinfer::util {

using TimePoint = std::chrono::_V2::system_clock::time_point;

TimePoint getTime();

class Timer {
 public:
  /**
   * @brief Construct a new Timer object
   *
   * @param time set the start time to this value
   */
  explicit Timer(TimePoint time);
  /**
   * @brief Construct a new Timer object
   *
   * @param start start the timer on construction
   */
  explicit Timer(bool start = false);

  /**
   * @brief Add or replace the label "start" with the current time
   *
   */
  void start();
  /**
   * @brief Add or replace the label "stop" with the current time
   *
   */
  void stop();
  /**
   * @brief Add or replace the given label with the current time
   *
   * @param label label to assign to the current timestamp
   */
  void add(const std::string& label);
  /**
   * @brief Add or replace the given label with the given time
   *
   * @param label label to assign to the timestamp
   * @param time the timestamp
   */
  void add(const std::string& label, TimePoint time);

  /**
   * @brief Clear all the saved timestamps
   *
   */
  void clear();

  /**
   * @brief Return the duration between two labeled timestamps
   *
   * @tparam U the ratio to convert the time e.g. std::micro for microseconds
   * @tparam T the type to return the time as
   * @param start the label for the start time
   * @param stop the label for the stop time
   * @return T the duration between the start and stop time in the right units
   */
  template <typename U = std::ratio<1, 1>, typename T = double>
  T count(const std::string& start = "start",
          const std::string& stop = "stop") {
    const auto& start_time = times_.at(start);
    const auto& stop_time = times_.at(stop);

    auto duration = std::chrono::duration_cast<std::chrono::duration<T, U>>(
      stop_time - start_time);
    return duration.count();
  }

 private:
  std::unordered_map<std::string, TimePoint> times_;
};

}  // namespace amdinfer::util

#endif  // GUARD_AMDINFER_UTIL_TIMER
