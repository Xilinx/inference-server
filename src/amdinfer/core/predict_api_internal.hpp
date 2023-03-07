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
 * @brief Defines the internal objects used to hold inference requests and
 * responses
 */

#ifndef GUARD_AMDINFER_CORE_PREDICT_API_INTERNAL
#define GUARD_AMDINFER_CORE_PREDICT_API_INTERNAL

#include "amdinfer/core/predict_api.hpp"  // IWYU pragma: export

namespace amdinfer {

struct RequestContainer {
  InferenceRequestPtr request;
#ifdef AMDINFER_ENABLE_TRACING
  TracePtr trace;
#endif
#ifdef AMDINFER_ENABLE_METRICS
  std::chrono::_V2::system_clock::time_point start_time;
#endif
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_PREDICT_API_INTERNAL
