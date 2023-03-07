// Copyright 2021 Xilinx, Inc.
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
 * @brief Implements the base ProtocolWrapper class
 */

#include "amdinfer/core/protocol_wrapper.hpp"

#include <utility>  // for move

#include "amdinfer/build_options.hpp"        // for AMDINFER_ENABLE_LOGGING
#include "amdinfer/observation/tracing.hpp"  // for TracePtr

namespace amdinfer {

#ifdef AMDINFER_ENABLE_TRACING
void ProtocolWrapper::setTrace(TracePtr&& trace) {
  this->trace_ = std::move(trace);
}
TracePtr&& ProtocolWrapper::getTrace() { return std::move(this->trace_); }
#endif

#ifdef AMDINFER_ENABLE_METRICS
void ProtocolWrapper::setTime(
  const std::chrono::high_resolution_clock::time_point& start_time) {
  this->start_time_ = start_time;
}

std::chrono::high_resolution_clock::time_point ProtocolWrapper::getTime()
  const {
  return this->start_time_;
}
#endif

#ifdef AMDINFER_ENABLE_LOGGING
const Logger& ProtocolWrapper::getLogger() const { return this->logger_; }
#endif

}  // namespace amdinfer
