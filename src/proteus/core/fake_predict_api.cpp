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

#include "proteus/core/fake_predict_api.hpp"

#include <memory>  // for make_unique
#include <string>  // for string
#include <vector>  // for vector

namespace proteus {

FakeInferenceRequest::FakeInferenceRequest(
  InferenceRequestInput& req, size_t& buffer_index,
  std::vector<BufferRawPtrs> input_buffers, std::vector<size_t>& input_offsets,
  std::vector<BufferRawPtrs> output_buffers,
  std::vector<size_t>& output_offsets, const size_t& batch_size,
  size_t& batch_offset) {
  this->id_ = "";
  this->parameters_ = std::make_unique<RequestParameters>();
  this->callback_ = nullptr;

  this->inputs_.emplace_back();
  this->outputs_.emplace_back();

  (void)req;
  (void)buffer_index;
  (void)input_buffers;
  (void)input_offsets;
  (void)output_buffers;
  (void)output_offsets;
  (void)batch_size;
  (void)batch_offset;
}

FakeInferenceRequest::FakeInferenceRequest() {
  this->id_ = "";
  this->parameters_ = std::make_unique<RequestParameters>();
  this->callback_ = nullptr;

  this->inputs_.emplace_back();
  this->outputs_.emplace_back();
}

}  // namespace proteus
