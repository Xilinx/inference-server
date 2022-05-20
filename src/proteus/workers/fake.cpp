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
 * @brief Implements a fake worker for testing
 */

#include <chrono>   // for chrono_literals
#include <cstddef>  // for size_t, byte
#include <cstdint>  // for uint32_t, int32_t
#include <memory>   // for unique_ptr, allocator
#include <mutex>    // for mutex, lock_guard
#include <thread>   // for thread, sleep_for
#include <utility>  // for move
#include <vector>   // for vector

#include "proteus/batching/fake_batcher.hpp"  // for FakeBatcher
#include "proteus/buffers/vector_buffer.hpp"  // for VectorBuffer
#include "proteus/build_options.hpp"          // for PROTEUS_ENABLE_TRACING
#include "proteus/core/data_types.hpp"        // for DataType, DataType::UINT32
#include "proteus/core/predict_api.hpp"       // for InferenceResponse, Requ...
#include "proteus/helpers/ctpl.h"             // for thread_pool
#include "proteus/helpers/declarations.hpp"   // for BufferPtr, InferenceRes...
#include "proteus/helpers/thread.hpp"         // for setThreadName
#include "proteus/observation/logging.hpp"    // for SPDLOG_LOGGER_INFO, SPD...
#include "proteus/observation/tracing.hpp"    // for startFollowSpan, SpanPtr
#include "proteus/workers/worker.hpp"         // for Worker

// define a duration of time to sleep. May be overidden at compile-time
#ifndef DURATION
#define DURATION 1ms
#endif

using namespace std::chrono_literals;

namespace proteus {

namespace workers {

/**
 * @brief The Fake worker is a worker for testing that does nothing but consume
 * incoming batches with a compile-time sleep duration. It responds with an
 * empty response.
 *
 */
class Fake : public Worker {
 public:
  using Worker::Worker;
  std::thread spawn(BatchPtrQueue* input_queue) override;

 private:
  void doInit(RequestParameters* parameters) override;
  size_t doAllocate(size_t num) override;
  void doAcquire(RequestParameters* parameters) override;
  void doRun(BatchPtrQueue* input_queue) override;
  void doRelease() override;
  void doDeallocate() override;
  void doDestroy() override;

  using Worker::makeBatcher;
  std::vector<std::unique_ptr<Batcher>> makeBatcher(int num = 1) override {
    return this->makeBatcher<FakeBatcher>(num);
  };

  ctpl::thread_pool pool_;
  std::mutex mutex_;
};

std::thread Fake::spawn(BatchPtrQueue* input_queue) {
  return std::thread(&Fake::run, this, input_queue);
}

void Fake::doInit(RequestParameters* parameters) {
  constexpr auto kMaxBufferNum = 50;

  auto max_buffer_num = kMaxBufferNum;
  if (parameters->has("max_buffer_num")) {
    max_buffer_num = parameters->get<int32_t>("max_buffer_num");
  }
  this->max_buffer_num_ = max_buffer_num;

  this->batch_size_ = 4;
}

size_t Fake::doAllocate(size_t num) {
  constexpr auto kBufferNum = 10U;
  size_t buffer_num =
    static_cast<int>(num) == kNumBufferAuto ? kBufferNum : num;
  VectorBuffer::allocate(this->input_buffers_, buffer_num,
                         1 * this->batch_size_, DataType::UINT32);
  VectorBuffer::allocate(this->output_buffers_, buffer_num,
                         1 * this->batch_size_, DataType::UINT32);
  return buffer_num;
}

void Fake::doAcquire(RequestParameters* parameters) {
  constexpr auto kThreads = 3;

  auto threads = kThreads;
  if (parameters->has("threads")) {
    threads = parameters->get<int32_t>("threads");
  }
  this->pool_.resize(threads);

  this->metadata_.addInputTensor("input", DataType::UINT32, {1});
  this->metadata_.addOutputTensor("output", DataType::UINT32, {1});
}

void Fake::doRun(BatchPtrQueue* input_queue) {
  std::shared_ptr<InferenceRequest> req;
  setThreadName("Fake");

  while (true) {
    BatchPtr batch;
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }
    SPDLOG_LOGGER_INFO(this->logger_, "Got request in fake");
    this->pool_.push([this, batch = std::move(batch)](int id) {
      (void)id;  // suppress unused variable warning

      std::vector<InferenceResponse> responses;
      responses.reserve(batch->requests->size());

      // int tensor_count = 0;
      for (unsigned int j = 0; j < batch->requests->size(); j++) {
        auto& req = batch->requests->at(j);
#ifdef PROTEUS_ENABLE_TRACING
        auto& trace = batch->traces.at(j);
        trace->startSpan("fake");
#endif
        auto& resp = responses.emplace_back();
        resp.setID("");
        resp.setModel("fake");
        (void)req;
      }

      {
        std::lock_guard<std::mutex> guard(this->mutex_);
        std::this_thread::sleep_for(DURATION);
      }

      for (unsigned int k = 0; k < batch->requests->size(); k++) {
        auto req = (*batch->requests)[k];
        auto inputs = req->getInputs();
        auto outputs = req->getOutputs();
        auto& resp = responses[k];

        auto output =
          InferenceResponseOutput(nullptr, {1}, DataType::UINT32, "fake");
        auto buffer = std::make_shared<std::vector<uint32_t>>();
        buffer->resize(1);
        (*buffer)[0] = 0;
        auto my_data_cast = std::reinterpret_pointer_cast<std::byte>(buffer);
        output.setData(std::move(my_data_cast));
        resp.addOutput(output);

#ifdef PROTEUS_ENABLE_TRACING
        auto context = batch->traces.at(k)->propagate();
        resp.setContext(std::move(context));
#endif

        req->runCallbackOnce(resp);
      }
      this->returnBuffers(std::move(batch->input_buffers),
                          std::move(batch->output_buffers));
      SPDLOG_LOGGER_DEBUG(this->logger_, "Returned buffers");
    });
  }
  SPDLOG_LOGGER_INFO(this->logger_, "Fake ending");
}

void Fake::doRelease() {}
void Fake::doDeallocate() { this->pool_.stop(true); }
void Fake::doDestroy() {}

}  // namespace workers

}  // namespace proteus

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
proteus::workers::Worker* getWorker() {
  return new proteus::workers::Fake("Fake", "CPU");
}
}  // extern C
