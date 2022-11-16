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

#ifndef GUARD_PROTEUS_OBSERVATION_OBSERVER
#define GUARD_PROTEUS_OBSERVATION_OBSERVER

#include "amdinfer/build_options.hpp"
#include "amdinfer/core/data_types.hpp"
#include "amdinfer/util/string.hpp"

#ifdef PROTEUS_ENABLE_LOGGING
#include "amdinfer/observation/logging.hpp"  // IWYU pragma: export
#endif

#ifdef PROTEUS_ENABLE_METRICS
#include "amdinfer/observation/metrics.hpp"  // IWYU pragma: export
#endif

#ifdef PROTEUS_ENABLE_TRACING
#include "amdinfer/observation/tracing.hpp"  // IWYU pragma: export
#endif

namespace amdinfer {

const auto kNumTraceData = 5U;

struct Observer {
  PROTEUS_IF_LOGGING(Logger logger);
};

inline void logTraceBuffer(Logger logger, void* data,
                           size_t size = sizeof(char)) {
  std::string bytes;
  const auto kBytes = kNumTraceData * size;
  // 6 for four characters for the byte (e.g. -128) + two for ", "
  const auto kBytesLength = kBytes * 6;
  bytes.reserve(kBytesLength);
  for (auto i = 0U; i < kBytes; ++i) {
    const auto* addr = static_cast<int8_t*>(data);
    bytes += std::to_string(static_cast<int>(addr[i])) + ", ";
  }
  PROTEUS_LOG_TRACE(
    logger, "Buffer(" + util::addressToString(data) + ") has bytes: " + bytes);
}

}  // namespace amdinfer

#endif  // GUARD_PROTEUS_OBSERVATION_OBSERVER
