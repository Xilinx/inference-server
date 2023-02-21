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

#ifndef GUARD_AMDINFER_OBSERVATION_OBSERVER
#define GUARD_AMDINFER_OBSERVATION_OBSERVER

#include "amdinfer/build_options.hpp"
#include "amdinfer/core/data_types.hpp"
#include "amdinfer/util/string.hpp"

#ifdef AMDINFER_ENABLE_LOGGING
#include "amdinfer/observation/logging.hpp"  // IWYU pragma: export
#endif

#ifdef AMDINFER_ENABLE_METRICS
#include "amdinfer/observation/metrics.hpp"  // IWYU pragma: export
#endif

#ifdef AMDINFER_ENABLE_TRACING
#include "amdinfer/observation/tracing.hpp"  // IWYU pragma: export
#endif

namespace amdinfer {

const auto kNumTraceData = 5U;

struct Observer {
  AMDINFER_IF_LOGGING(Logger logger);
};

inline void logTraceBuffer([[maybe_unused]] const Logger& logger, void* data,
                           size_t size = sizeof(char)) {
  std::string bytes;
  const auto num_bytes = kNumTraceData * size;
  // 6 for four characters for the byte (e.g. -128) + two for ", "
  const auto bytes_length = num_bytes * 6;
  bytes.reserve(bytes_length);
  for (auto i = 0U; i < num_bytes; ++i) {
    const auto* addr = static_cast<int8_t*>(data);
    bytes += std::to_string(static_cast<int>(addr[i])) + ", ";
  }
  AMDINFER_LOG_TRACE(
    logger, "Buffer(" + util::addressToString(data) + ") has bytes: " + bytes);
}

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_OBSERVATION_OBSERVER
