// Copyright 2021 Xilinx Inc.
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
 * @brief Implements the base Interface class
 */

#include "proteus/core/interface.hpp"

#include <utility>  // for move

#include "proteus/build_options.hpp"        // for PROTEUS_ENABLE_LOGGING
#include "proteus/observation/tracing.hpp"  // for TracePtr

namespace proteus {

Interface::Interface() { this->type_ = InterfaceType::kUnknown; }

InterfaceType Interface::getType() const { return this->type_; }

#ifdef PROTEUS_ENABLE_TRACING
void Interface::setTrace(TracePtr&& trace) { this->trace_ = std::move(trace); }
TracePtr&& Interface::getTrace() { return std::move(this->trace_); }
#endif

#ifdef PROTEUS_ENABLE_METRICS
void Interface::set_time(
  const std::chrono::high_resolution_clock::time_point& start_time) {
  this->start_time_ = start_time;
}

std::chrono::high_resolution_clock::time_point Interface::get_time() const {
  return this->start_time_;
}
#endif

#ifdef PROTEUS_ENABLE_LOGGING
const Logger& Interface::getLogger() const { return this->logger_; }
#endif

}  // namespace proteus
