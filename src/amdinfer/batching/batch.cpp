// Copyright 2023 Advanced Micro Devices, Inc.
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
 * @brief
 */

#include "amdinfer/batching/batch.hpp"

#include <cassert>

#include "amdinfer/buffers/buffer.hpp"
#include "amdinfer/observation/tracing.hpp"

namespace amdinfer {

void Batch::addRequest(InferenceRequestPtr request) {
  requests_.push_back(std::move(request));
  // models_.emplace_back();
}

std::unique_ptr<Batch> Batch::propagate() {
  const auto batch_size = this->size();
  auto new_batch = std::make_unique<Batch>();
  new_batch->models_.resize(batch_size);

  for (auto i = 0U; i < batch_size; ++i) {
    new_batch->setModel(i, this->getModel(i));
#ifdef AMDINFER_ENABLE_METRICS
    new_batch->addTime(this->getTime(i));
#endif
  }
  return new_batch;
}

const BufferPtrs& Batch::getInputBuffers() const { return input_buffers_; }

const BufferPtrs& Batch::getOutputBuffers() const { return output_buffers_; }

const std::vector<InferenceRequestPtr>& Batch::getRequests() const {
  return requests_;
}

const InferenceRequestPtr& Batch::getRequest(size_t index) {
  return requests_.at(index);
}

bool Batch::empty() const { return requests_.empty(); }

size_t Batch::size() const {
  // assert(requests_.size() == models_.size());

  return requests_.size();
}

size_t Batch::getInputSize() const { return input_buffers_.size(); }

size_t Batch::getOutputSize() const { return output_buffers_.size(); }

void Batch::setBuffers(BufferPtrs inputs, BufferPtrs outputs) {
  input_buffers_ = std::move(inputs);
  output_buffers_ = std::move(outputs);
}

const std::string& Batch::getModel(size_t index) const {
  return models_.at(index);
}

void Batch::setModel(size_t index, std::string model) {
  auto& old_model = models_.at(index);
  if (old_model.empty()) {
    old_model = std::move(model);
  }
}

void Batch::addModel(std::string model) {
  models_.emplace_back(std::move(model));
}

#ifdef AMDINFER_ENABLE_TRACING
void Batch::addTrace(TracePtr trace) { traces_.push_back(std::move(trace)); }

TracePtr& Batch::getTrace(size_t index) { return traces_.at(index); }
#endif

#ifdef AMDINFER_ENABLE_METRICS
void Batch::addTime(std::chrono::high_resolution_clock::time_point timestamp) {
  start_times_.push_back(timestamp);
}

std::chrono::high_resolution_clock::time_point Batch::getTime(size_t index) {
  return start_times_.at(index);
}
#endif

}  // namespace amdinfer
