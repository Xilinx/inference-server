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
 * @brief Implements a fake WorkerInfo class - most methods are empty but the
 * worker has a queue that can be manually populated for providing buffers
 */

#include <cstddef>  // for size_t
#include <cstdint>  // for int32_t
#include <memory>   // for unique_ptr, make_unique
#include <string>   // for allocator, string
#include <thread>   // for thread::id, thread
#include <utility>  // for move

#include "proteus/batching/batcher.hpp"  // for Batcher
#include "proteus/buffers/buffer.hpp"    // IWYU pragma: keep
#include "proteus/core/predict_api.hpp"  // for RequestParameters
#include "proteus/core/worker_info.hpp"  // for WorkerInfo
#include "proteus/declarations.hpp"      // for BufferPtrs
#include "proteus/util/queue.hpp"        // for BufferPtrsQueue, Blockin...

namespace proteus {

WorkerInfo::WorkerInfo(const std::string& name, RequestParameters* parameters) {
  this->input_buffer_ptr_ = std::make_unique<BufferPtrsQueue>();
  this->output_buffer_ptr_ = std::make_unique<BufferPtrsQueue>();
  this->buffer_num_ = 10;
  this->max_buffer_num_ = 10;
  this->batch_size_ = 1;

  this->addAndStartWorker(name, parameters);
}

WorkerInfo::~WorkerInfo() {}

void WorkerInfo::addAndStartWorker(const std::string& name,
                                   RequestParameters* parameters) {
  (void)name;
  if (parameters == nullptr) {
    return;
  }
  if (parameters->has("batch_size")) {
    this->batch_size_ = parameters->get<int32_t>("batch_size");
  }
}

Batcher* WorkerInfo::getBatcher() { return nullptr; }

void WorkerInfo::join(std::thread::id id) { (void)id; }

void WorkerInfo::joinAll() {}

void WorkerInfo::unload() {}

size_t WorkerInfo::getGroupSize() const { return 1; }

void WorkerInfo::shutdown() {}

BufferPtrs WorkerInfo::getInputBuffer() const {
  BufferPtrs buffer;
  input_buffer_ptr_->wait_dequeue(buffer);
  return buffer;
}

BufferPtrs WorkerInfo::getOutputBuffer() const {
  BufferPtrs buffer;
  output_buffer_ptr_->wait_dequeue(buffer);
  return buffer;
}

void WorkerInfo::putInputBuffer(BufferPtrs buffer) const {
  this->input_buffer_ptr_->enqueue(std::move(buffer));
}

void WorkerInfo::putOutputBuffer(BufferPtrs buffer) const {
  this->output_buffer_ptr_->enqueue(std::move(buffer));
}

bool WorkerInfo::inputSizeValid(size_t size) const {
  (void)size;
  return true;
}

void WorkerInfo::allocate(size_t request_size) { (void)request_size; }

}  // namespace proteus
