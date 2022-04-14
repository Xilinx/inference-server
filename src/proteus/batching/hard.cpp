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
 * @brief Implements the hard batcher
 */

#include "proteus/batching/hard.hpp"

#include <chrono>     // for system_clock::time_point
#include <cstddef>    // for size_t
#include <memory>     // for unique_ptr, make_unique
#include <mutex>      // for mutex, unique_lock
#include <stdexcept>  // for invalid_argument
#include <string>     // for operator+
#include <utility>    // for move
#include <vector>     // for vector

#include "proteus/buffers/buffer.hpp"        // IWYU pragma: keep
#include "proteus/build_options.hpp"         // for PROTEUS_ENABLE_TRACING
#include "proteus/core/interface.hpp"        // for InterfacePtr, Interface
#include "proteus/core/manager.hpp"          // for Manager
#include "proteus/core/worker_info.hpp"      // for WorkerInfo
#include "proteus/helpers/declarations.hpp"  // for BufferPtr, InferenceRequ...
#include "proteus/helpers/queue.hpp"         // for BlockingConcurrentQueue
#include "proteus/helpers/thread.hpp"        // for setThreadName
#include "proteus/observation/metrics.hpp"   // for Metrics
#include "proteus/observation/tracing.hpp"   // for startFollowSpan, SpanPtr

namespace proteus {
class InferenceRequest;
}  // namespace proteus
// IWYU pragma: no_forward_declare proteus::Buffer

namespace proteus {

void HardBatcher::doRun(WorkerInfo* worker) {
  auto thread_name = "batch" + this->getName();
  setThreadName(thread_name);
  std::unique_lock<std::mutex> lk(this->cv_m_);
  InterfacePtr req;
  bool run = true;

  while (run) {
    auto batch = std::make_unique<Batch>();
    batch->requests =
      std::make_unique<std::vector<std::shared_ptr<InferenceRequest>>>();
    batch->input_buffers = std::make_unique<std::vector<BufferPtrs>>();
    batch->output_buffers = std::make_unique<std::vector<BufferPtrs>>();
    auto input_buffer = worker->getInputBuffer();
    std::vector<size_t> input_offset = {0};
    auto output_buffer = worker->getOutputBuffer();
    std::vector<size_t> output_offset = {0};
    size_t batch_size = 0;

    do {
      std::vector<BufferRawPtrs> input_buffers;
      input_buffers.emplace_back();
      for (const auto& buffer : input_buffer) {
        input_buffers.back().push_back(buffer.get());
      }
      std::vector<BufferRawPtrs> output_buffers;
      output_buffers.emplace_back();
      for (const auto& buffer : output_buffer) {
        output_buffers.back().push_back(buffer.get());
      }
      this->input_queue_->wait_dequeue(req);

      if (req == nullptr) {
        run = false;
        break;
      }

#ifdef PROTEUS_ENABLE_TRACING
      auto trace = req->getTrace();
      trace->startSpan("hard_batcher");
#endif

#ifdef PROTEUS_ENABLE_METRICS
      Metrics::getInstance().incrementCounter(
        MetricCounterIDs::kPipelineIngressBatcher);
#endif

      size_t input_size = 0;
      try {
        input_size = req->getInputSize();
        if (!worker->inputSizeValid(input_size)) {
          Manager::getInstance().workerAllocate(this->model_, input_size);
        }
      } catch (const std::invalid_argument& e) {
        req->errorHandler(e);
        continue;
      }
      if (input_size == 0) {
        auto error = std::invalid_argument("Input size is zero");
        req->errorHandler(error);
        continue;
      }
      auto buffers_needed =
        ((input_size + batch_size - 1) / this->batch_size_) + 1;
      if (buffers_needed > 1) {
        for (size_t i = 1; i < buffers_needed; i++) {
          batch->input_buffers->push_back(std::move(input_buffer));
          batch->output_buffers->push_back(std::move(output_buffer));
          input_buffer = worker->getInputBuffer();
          // input_buffers.push_back(input_buffer.get());
          input_buffers.emplace_back();
          for (const auto& buffer : input_buffer) {
            input_buffers.back().push_back(buffer.get());
          }
          output_buffer = worker->getOutputBuffer();
          // output_buffers.push_back(output_buffer.get());
          output_buffers.emplace_back();
          for (const auto& buffer : output_buffer) {
            output_buffers.back().push_back(buffer.get());
          }
          input_offset.push_back(0);
          output_offset.push_back(0);
        }
      }

      auto old_input_offset = input_offset;
      auto old_output_offset = output_offset;
      size_t buffer_index = 0;
      auto new_req = req->getRequest(buffer_index, input_buffers, input_offset,
                                     output_buffers, output_offset,
                                     this->batch_size_, batch_size);
      if (new_req == nullptr) {
        // if the request fails, revert back the state
        for (size_t i = 1; i < buffers_needed; i++) {
          worker->putInputBuffer(std::move(input_buffer));
          input_buffer = std::move(batch->input_buffers->back());
          batch->input_buffers->pop_back();

          worker->putOutputBuffer(std::move(output_buffer));
          output_buffer = std::move(batch->output_buffers->back());
          batch->output_buffers->pop_back();

          input_buffers.pop_back();
          output_buffers.pop_back();
        }
        input_offset = old_input_offset;
        output_offset = old_output_offset;
      } else {
        batch->requests->push_back(new_req);
#ifdef PROTEUS_ENABLE_TRACING
        trace->endSpan();
        batch->traces.emplace_back(std::move(trace));
#endif
#ifdef PROTEUS_ENABLE_METRICS
        batch->start_times.emplace_back(req->get_time());
#endif
      }
    } while (batch_size % this->batch_size_ != 0);

    if (!batch->requests->empty()) {
      batch->input_buffers->push_back(std::move(input_buffer));
      batch->output_buffers->push_back(std::move(output_buffer));
      this->output_queue_->enqueue(std::move(batch));
#ifdef PROTEUS_ENABLE_METRICS
      Metrics::getInstance().incrementCounter(
        MetricCounterIDs::kPipelineEgressBatcher);
#endif
    } else {
      worker->putInputBuffer(std::move(input_buffer));
      worker->putOutputBuffer(std::move(output_buffer));
    }
  }
}

}  // namespace proteus
