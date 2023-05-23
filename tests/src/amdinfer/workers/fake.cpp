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
 * @brief Implements the Fake worker
 */

#include <dlfcn.h>  // for dlopen

#include <array>    // for array
#include <cassert>  // for assert
#include <cstddef>  // for size_t, byte
#include <cstdint>  // for uint32_t, int32_t
#include <cstring>  // for memcpy
#include <memory>   // for unique_ptr, allocator
#include <ratio>    // for micro
#include <string>   // for string
#include <thread>   // for thread
#include <utility>  // for move
#include <vector>   // for vector

#include "amdinfer/batching/soft.hpp"    // for SoftBatcher
#include "amdinfer/build_options.hpp"    // for AMDINFER_ENABLE_TRACING
#include "amdinfer/core/data_types.hpp"  // for DataType, DataType::Uint32
#include "amdinfer/core/inference_request.hpp"  // for InferenceRequest, Infe...
#include "amdinfer/core/inference_response.hpp"  // for InferenceResponse
#include "amdinfer/core/parameters.hpp"          // for ParameterMap
#include "amdinfer/declarations.hpp"         // for BufferPtr, InferenceRes...
#include "amdinfer/observation/logging.hpp"  // for Logger
#include "amdinfer/observation/metrics.hpp"  // for Metrics
#include "amdinfer/observation/tracing.hpp"  // for startFollowSpan, SpanPtr
#include "amdinfer/util/containers.hpp"      // for containerSum
#include "amdinfer/util/queue.hpp"           // for BufferPtrsQueue
#include "amdinfer/util/string.hpp"          // for endsWith
#include "amdinfer/util/thread.hpp"          // for setThreadName
#include "amdinfer/util/timer.hpp"           // for Timer
#include "amdinfer/workers/worker.hpp"       // for Worker

namespace amdinfer::workers {

/**
 * @brief The Fake worker blocks for a configurable amount of time and is used
 * for benchmarking purposes
 *
 */
class Fake : public SingleThreadedWorker {
 public:
  using SingleThreadedWorker::SingleThreadedWorker;
  [[nodiscard]] std::vector<MemoryAllocators> getAllocators() const override;

 private:
  int delay_ = 0;
  int loop_ = 0;

  void doInit(ParameterMap* parameters) override;
  void doAcquire(ParameterMap* parameters) override;
  BatchPtr doRun(Batch* batch, const MemoryPool* pool) override;
  void doRelease() override;
  void doDestroy() override;

  // workers define what batcher implementation should be used for them.
  // if not explicitly defined here, a default value is used from worker.hpp.
  using Worker::makeBatcher;
  std::vector<std::unique_ptr<Batcher>> makeBatcher(int num,
                                                    ParameterMap* parameters,
                                                    MemoryPool* pool) override {
    return this->makeBatcher<SoftBatcher>(num, parameters, pool);
  };
};

std::vector<MemoryAllocators> Fake::getAllocators() const {
  return {MemoryAllocators::Cpu};
}

void Fake::doInit(ParameterMap* parameters) {
  if (parameters->has("delay")) {
    delay_ = parameters->get<int32_t>("delay");
  }

  if (parameters->has("loop")) {
    loop_ = parameters->get<int32_t>("loop");
  }
}

void Fake::doAcquire([[maybe_unused]] ParameterMap* parameters) {
  // nothing to acquire
}

BatchPtr Fake::doRun([[maybe_unused]] Batch* batch,
                     [[maybe_unused]] const MemoryPool* pool) {
  auto new_batch = batch->propagate();
  new_batch->addModel("fake");

  std::this_thread::sleep_for(std::chrono::milliseconds(delay_));

  // busy loop to prevent optimization
  for (auto i = 0; i < loop_; ++i) {
    __asm__ __volatile__("" : "+g"(i) : :);
  }

  const auto& old_requests = batch->getRequests();
  for (const auto& old_request : old_requests) {
    new_batch->addRequest(old_request);
  }

  return new_batch;
}

void Fake::doRelease() {}
void Fake::doDestroy() {}

}  // namespace amdinfer::workers

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
amdinfer::workers::Worker* getWorker() {
  return new amdinfer::workers::Fake("Fake", "CPU", true);
}
}  // extern C
