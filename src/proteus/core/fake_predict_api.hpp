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

#ifndef GUARD_PROTEUS_CORE_FAKE_PREDICT_API
#define GUARD_PROTEUS_CORE_FAKE_PREDICT_API

#include <cstddef>  // for size_t
#include <vector>   // for vector

#include "proteus/core/predict_api.hpp"  // for InferenceRequest, InferenceR...

namespace proteus {
class Buffer;
}  // namespace proteus

namespace proteus {

class FakeInferenceRequest : public InferenceRequest {
 public:
  FakeInferenceRequest();
  FakeInferenceRequest(InferenceRequestInput& req, size_t& buffer_index,
                       std::vector<BufferRawPtrs> input_buffers,
                       std::vector<size_t>& input_offsets,
                       std::vector<BufferRawPtrs> output_buffers,
                       std::vector<size_t>& output_offsets,
                       const size_t& batch_size, size_t& batch_offset);
};

}  // namespace proteus

#endif  // GUARD_PROTEUS_CORE_FAKE_PREDICT_API
