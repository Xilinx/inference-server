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
 * @brief Defines the fake predict API InferenceRequest
 */

#ifndef GUARD_PROTEUS_CORE_FAKE_PREDICT_API
#define GUARD_PROTEUS_CORE_FAKE_PREDICT_API

#include <cstddef>  // for size_t
#include <vector>   // for vector

#include "proteus/core/predict_api.hpp"      // for InferenceRequest, Infere...
#include "proteus/helpers/declarations.hpp"  // for BufferRawPtrs

namespace proteus {

/**
 * @brief The FakeInferenceRequest object is used to mock the real thing for
 * testing purposes.
 *
 */
class FakeInferenceRequest : public InferenceRequest {
 public:
  FakeInferenceRequest();  ///< Constructor
  FakeInferenceRequest(InferenceRequestInput& req,
                       const BufferRawPtrs& input_buffers,
                       std::vector<size_t>& input_offsets,
                       const BufferRawPtrs& output_buffers,
                       std::vector<size_t>& output_offsets);
};

}  // namespace proteus

#endif  // GUARD_PROTEUS_CORE_FAKE_PREDICT_API
