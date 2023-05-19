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
 * @brief Implements the soft batcher
 */

#include "amdinfer/batching/soft.hpp"

#include <algorithm>  // for max
#include <cstddef>    // for size_t
#include <cstdint>    // for int32_t
#include <memory>     // for unique_ptr, allocator
#include <ratio>      // for ratio
#include <string>     // for operator+, char_traits
#include <utility>    // for move
#include <vector>     // for vector

#include "amdinfer/buffers/cpu.hpp"             // for CpuBuffer
#include "amdinfer/build_options.hpp"           // for AMDINFER_ENABLE_METRICS
#include "amdinfer/core/exceptions.hpp"         // for invalid_argument
#include "amdinfer/core/inference_request.hpp"  // for InferenceRequest
#include "amdinfer/core/memory_pool/pool.hpp"
#include "amdinfer/core/parameters.hpp"         // for ParameterMap
#include "amdinfer/core/request_container.hpp"  // for InferenceRequestInput
#include "amdinfer/core/worker_info.hpp"
#include "amdinfer/declarations.hpp"         // for RequestContainerPtr
#include "amdinfer/observation/logging.hpp"  // for Logger, AMDINFER_LOG_DEBUG
#include "amdinfer/observation/metrics.hpp"  // for Metrics, MetricCounterIDs
#include "amdinfer/observation/tracing.hpp"  // for Trace
#include "amdinfer/util/queue.hpp"           // for BlockingConcurrentQueue
#include "amdinfer/util/thread.hpp"          // for setThreadName
#include "amdinfer/util/timer.hpp"           // for Timer

// default batcher timeout in milliseconds
constexpr auto kDefaultTimeout = 100;

namespace amdinfer {

void SoftBatcher::doRun(const std::vector<MemoryAllocators>& allocators) {
  auto thread_name = "batch" + this->getName();
  util::setThreadName(thread_name);
#ifdef AMDINFER_ENABLE_LOGGING
  [[maybe_unused]] const auto& logger = this->getLogger();
#endif

  bool run = true;

  auto timeout = kDefaultTimeout;
  if (this->parameters_.has("timeout")) {
    timeout = this->parameters_.get<int32_t>("timeout");
  }

  while (run) {
    auto batch = std::make_unique<Batch>();
    size_t batch_size = 0;

    std::vector<size_t> input_offset;
    std::vector<size_t> output_offset;

#ifdef AMDINFER_ENABLE_METRICS
    Metrics::getInstance().setGauge(
      MetricGaugeIDs::QueuesBatcherInput,
      static_cast<double>(input_queue_->size_approx()));
    Metrics::getInstance().setGauge(
      MetricGaugeIDs::QueuesBatcherOutput,
      static_cast<double>(output_queue_->size_approx()));
#endif

    bool first_request = true;
    util::Timer timer{true};

    do {
      RequestContainerPtr req;
      if (first_request) {
        // wait for the first request
        this->input_queue_->wait_dequeue(req);
        timer.add("start");
        AMDINFER_LOG_DEBUG(logger,
                           "Got request of a new batch for " + this->model_);
      } else {
        timer.stop();

        auto remaining_time = timeout - timer.count<std::milli, int>();
        // convert duration from milliseconds to microseconds for function
        auto duration = std::max(remaining_time, 0) * std::kilo::num;
        bool valid = this->input_queue_->wait_dequeue_timed(req, duration);
        if (!valid) {
          break;
        }
      }

      if (req == nullptr) {
        run = false;
        break;
      }

      auto request = req->request;
      const auto& inputs = request->getInputs();
      auto input_size = inputs.size();
      if (input_size == 0) {
        request->runCallbackError("Input size is zero");
        continue;
      }

      if (first_request) {
        std::vector<BufferPtr> input_buffers;
        input_buffers.reserve(input_size);
        // auto output_sizes = req->getOutputSizes();
        // TODO(varunsh): the spec does not require the request to have outputs
        // additionally, the output size could be variable so this should be
        // allocated by the worker

        // std::vector<BufferPtr> output_buffers;
        // output_buffers.reserve(output_sizes.size());
        // std::vector<size_t> output_offset(output_buffers.size(), 0);
        for (const auto& input : inputs) {
          input_buffers.push_back(pool_->get(allocators, input, batch_size_));
        }
        // for(const auto& tensor_size : output_sizes) {
        //   output_buffers.push_back(pool_->get(allocators, tensor_size));
        // }
        input_offset.resize(input_buffers.size());
        batch->setBuffers(std::move(input_buffers), {});
      }

      const auto& input_buffers = batch->getInputBuffers();

#ifdef AMDINFER_ENABLE_TRACING
      auto& trace = req->trace;
      trace->startSpan("soft_batcher");
#endif

#ifdef AMDINFER_ENABLE_METRICS
      Metrics::getInstance().incrementCounter(
        MetricCounterIDs::PipelineIngressBatcher);
#endif

      auto old_input_offset = input_offset;

      for (auto i = 0U; i < input_size; ++i) {
        const auto& input = inputs[i];
        const auto& input_buffer = input_buffers.at(i);
        auto& offset = input_offset[i];

        auto new_offset =
          input_buffer->write(input.getData(), offset,
                              input.getSize() * input.getDatatype().size());
        pool_->put(MemoryAllocators::Cpu, input.getData());
        request->setInputTensorData(i, input_buffer->data(offset));
        offset = new_offset;
      }

      batch->addRequest(request);
      batch_size++;
      batch->addModel("");
#ifdef AMDINFER_ENABLE_TRACING
      trace->endSpan();
      batch->addTrace(std::move(trace));
#endif
#ifdef AMDINFER_ENABLE_METRICS
      batch->addTime(req->start_time);
#endif
      first_request = false;
    } while (batch_size % this->batch_size_ != 0 && run);

    if (!batch->empty()) {
      AMDINFER_LOG_DEBUG(logger, "Enqueuing batch for " + this->model_ +
                                   " of size " + std::to_string(batch_size));
      this->output_queue_->enqueue(std::move(batch));
#ifdef AMDINFER_ENABLE_METRICS
      Metrics::getInstance().incrementCounter(
        MetricCounterIDs::PipelineEgressBatcher);
#endif
    }
  }
}

}  // namespace amdinfer
