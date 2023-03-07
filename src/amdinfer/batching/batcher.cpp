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

#include "amdinfer/buffers/buffer.hpp"             // IWYU pragma: keep
#include "amdinfer/core/memory_pool/pool.hpp"      // for MemoryPool
#include "amdinfer/core/predict_api_internal.hpp"  // for InferenceRequestInput
#include "amdinfer/core/worker_info.hpp"           // for WorkerInfo
#include "amdinfer/observation/logging.hpp"  // for Logger, Loggers, Logger...

namespace amdinfer {

/**
 * @brief The C++ ProtocolWrapper class encapsulates incoming requests from the
 * C++ API to the batcher.
 *
 */

Batcher::Batcher(MemoryPool* pool) : pool_(pool) {
  this->input_queue_ = std::make_shared<BlockingQueue<RequestContainerPtr>>();
  this->output_queue_ = std::make_shared<BatchPtrQueue>();
  this->status_ = BatcherStatus::New;
#ifdef AMDINFER_ENABLE_LOGGING
  this->logger_ = Logger(Loggers::Server);
#endif
}

Batcher::Batcher(MemoryPool* pool, ParameterMap* parameters) : Batcher(pool) {
  if (parameters != nullptr) {
    this->parameters_ = *parameters;
  }
}

Batcher::Batcher(const Batcher& batcher)
  : batch_size_(batcher.batch_size_),
    input_queue_(batcher.input_queue_),
    output_queue_(batcher.output_queue_),
    model_(batcher.model_),
    pool_(batcher.pool_) {
  this->status_ = BatcherStatus::New;
#ifdef AMDINFER_ENABLE_LOGGING
  this->logger_ = Logger(Loggers::Server);
#endif
}

void Batcher::start(const std::vector<MemoryAllocators>& allocators) {
  this->status_ = BatcherStatus::Run;
  this->thread_ = std::thread(&Batcher::run, this, allocators);
}

void Batcher::setBatchSize(size_t batch_size) {
  this->batch_size_ = batch_size;
}

void Batcher::setName(const std::string& name) { this->model_ = name; }

std::string Batcher::getName() const { return this->model_; }

BlockingQueue<RequestContainerPtr>* Batcher::getInputQueue() {
  return this->input_queue_.get();
}

BatchPtrQueue* Batcher::getOutputQueue() { return this->output_queue_.get(); }

void Batcher::enqueue(RequestContainerPtr request) const {
  this->input_queue_->enqueue(std::move(request));
}

void Batcher::run(const std::vector<MemoryAllocators>& allocators) {
  this->doRun(allocators);
  this->status_ = BatcherStatus::Inactive;
}

BatcherStatus Batcher::getStatus() const { return this->status_; }

void Batcher::end() {
  this->thread_.join();
  this->status_ = BatcherStatus::Dead;
}

#ifdef AMDINFER_ENABLE_LOGGING
const Logger& Batcher::getLogger() const { return logger_; }
#endif

}  // namespace amdinfer
