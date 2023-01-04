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
 * @brief Defines shared objects for AksDetect workers
 */

#ifndef GUARD_AMDINFER_WORKERS_AKS_DETECT
#define GUARD_AMDINFER_WORKERS_AKS_DETECT

namespace amdinfer::workers {

struct DetectResponse {
  float class_id;
  float score;
  float x;
  float y;
  float w;
  float h;
};

// the number of float values per response
const int kAkdDetectResponseSize = 7;

}  // namespace amdinfer::workers

#endif  // GUARD_AMDINFER_WORKERS_AKS_DETECT
