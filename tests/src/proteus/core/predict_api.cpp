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
 * @brief Implements the fake predict api InferenceRequest
 */

#include "proteus/core/predict_api.hpp"  // for InferenceRequestInput

#include <cstddef>  // for size_t
#include <memory>   // for make_unique
#include <string>   // for string
#include <vector>   // for vector

#include "proteus/core/fake_predict_api.hpp"  // for FakeInferenceRequest
#include "proteus/declarations.hpp"           // for BufferRawPtrs

namespace proteus {

FakeInferenceRequest::FakeInferenceRequest(
  InferenceRequestInput& req, const BufferRawPtrs& input_buffers,
  std::vector<size_t>& input_offsets, const BufferRawPtrs& output_buffers,
  std::vector<size_t>& output_offsets) {
  this->id_ = "";
  this->parameters_ = std::make_unique<RequestParameters>();
  this->callback_ = nullptr;

  this->inputs_.emplace_back();
  this->outputs_.emplace_back();

  (void)req;
  (void)input_buffers;
  (void)input_offsets;
  (void)output_buffers;
  (void)output_offsets;
}

FakeInferenceRequest::FakeInferenceRequest() {
  this->id_ = "";
  this->parameters_ = std::make_unique<RequestParameters>();
  this->callback_ = nullptr;

  this->inputs_.emplace_back();
  this->outputs_.emplace_back();
}

}  // namespace proteus
