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
 * @brief Implements the base batcher and CppNativeApi interface
 */

#include "amdinfer/batching/batcher.hpp"

#include <cassert>  // for assert
#include <memory>   // for shared_ptr, make_shared
#include <string>   // for string
#include <utility>  // for move

#include "amdinfer/buffers/buffer.hpp"       // IWYU pragma: keep
#include "amdinfer/core/interface.hpp"       // IWYU pragma: keep
#include "amdinfer/core/worker_info.hpp"     // for WorkerInfo
#include "amdinfer/observation/logging.hpp"  // for Logger, Loggers, Loggers:...

namespace amdinfer {

/**
 * @brief The C++ Interface class encapsulates incoming requests from the C++
 * API to the batcher.
 *
 */

Batcher::Batcher() {
  this->input_queue_ = std::make_shared<BlockingQueue<InterfacePtr>>();
  this->output_queue_ = std::make_shared<BatchPtrQueue>();
  this->status_ = BatcherStatus::kNew;
#ifdef AMDINFER_ENABLE_LOGGING
  this->logger_ = Logger(Loggers::kServer);
#endif
}

Batcher::Batcher(RequestParameters* parameters) : Batcher() {
  if (parameters != nullptr) {
    this->parameters_ = *parameters;
  }
}

Batcher::Batcher(const Batcher& batcher) {
  this->input_queue_ = batcher.input_queue_;
  this->output_queue_ = batcher.output_queue_;
  this->batch_size_ = batcher.batch_size_;
  this->status_ = BatcherStatus::kNew;
#ifdef AMDINFER_ENABLE_LOGGING
  this->logger_ = Logger(Loggers::kServer);
#endif
  this->model_ = batcher.model_;
}

void Batcher::start(WorkerInfo* worker) {
  this->status_ = BatcherStatus::kRun;
  this->thread_ = std::thread(&Batcher::run, this, worker);
}

void Batcher::setBatchSize(size_t batch_size) {
  this->batch_size_ = batch_size;
}

void Batcher::setName(const std::string& name) { this->model_ = name; }

std::string Batcher::getName() { return this->model_; }

BlockingQueue<InterfacePtr>* Batcher::getInputQueue() {
  return this->input_queue_.get();
}

BatchPtrQueue* Batcher::getOutputQueue() { return this->output_queue_.get(); }

void Batcher::enqueue(InterfacePtr request) {
  this->input_queue_->enqueue(std::move(request));
}

void Batcher::run(WorkerInfo* worker) {
  this->doRun(worker);
  this->status_ = BatcherStatus::kInactive;
}

BatcherStatus Batcher::getStatus() { return this->status_; }

void Batcher::end() {
  this->thread_.join();
  this->status_ = BatcherStatus::kDead;
}

#ifdef AMDINFER_ENABLE_LOGGING
const Logger& Batcher::getLogger() const { return logger_; }
#endif

Batch::Batch(const WorkerInfo* worker)
  : worker_(worker),
    input_buffers_(worker->getInputBuffer()),
    output_buffers_(worker->getOutputBuffer()) {}

Batch::~Batch() {
  worker_->putInputBuffer(std::move(input_buffers_));
  worker_->putOutputBuffer(std::move(output_buffers_));
}

void Batch::addRequest(InferenceRequestPtr request) {
  requests_.push_back(std::move(request));
}

std::vector<Buffer*> Batch::getRawInputBuffers() const {
  std::vector<Buffer*> buffers;
  buffers.reserve(input_buffers_.size());
  for (const auto& buffer : input_buffers_) {
    buffers.push_back(buffer.get());
  }
  return buffers;
}

const BufferPtrs& Batch::getInputBuffers() const { return input_buffers_; }

std::vector<Buffer*> Batch::getRawOutputBuffers() const {
  std::vector<Buffer*> buffers;
  buffers.reserve(output_buffers_.size());
  for (const auto& buffer : output_buffers_) {
    buffers.push_back(buffer.get());
  }
  return buffers;
}

const BufferPtrs& Batch::getOutputBuffers() const { return output_buffers_; }

const std::vector<InferenceRequestPtr>& Batch::getRequests() {
  return requests_;
}

const InferenceRequestPtr& Batch::getRequest(int index) {
  return requests_.at(index);
}

bool Batch::empty() const { return requests_.empty(); }

size_t Batch::size() const {
#ifdef AMDINFER_ENABLE_TRACING
  assert(requests_.size() == traces_.size());
#endif
#ifdef AMDINFER_ENABLE_METRICS
  assert(requests_.size() == start_times_.size());
#endif

  return requests_.size();
}

size_t Batch::input_size() const { return input_buffers_.size(); }

size_t Batch::output_size() const { return output_buffers_.size(); }

#ifdef AMDINFER_ENABLE_TRACING
void Batch::addTrace(TracePtr trace) { traces_.push_back(std::move(trace)); }

TracePtr& Batch::getTrace(int index) { return traces_.at(index); }
#endif

#ifdef AMDINFER_ENABLE_METRICS
void Batch::addTime(std::chrono::high_resolution_clock::time_point timestamp) {
  start_times_.push_back(std::move(timestamp));
}

std::chrono::high_resolution_clock::time_point Batch::getTime(int index) {
  return start_times_.at(index);
}
#endif

}  // namespace amdinfer
