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
 * @brief Implements the hard batcher
 */

#include "amdinfer/batching/hard.hpp"

#include <cstddef>  // for size_t
#include <memory>   // for unique_ptr, operator==
#include <string>   // for operator+
#include <utility>  // for move
#include <vector>   // for vector

#include "amdinfer/buffers/cpu.hpp"             // for CpuBuffer
#include "amdinfer/build_options.hpp"           // for AMDINFER_ENABLE_METRICS
#include "amdinfer/core/exceptions.hpp"         // for invalid_argument
#include "amdinfer/core/inference_request.hpp"  // for InferenceRequest
#include "amdinfer/core/memory_pool/pool.hpp"   // for MemoryPool
#include "amdinfer/core/request_container.hpp"  // for InferenceRequestInput
#include "amdinfer/core/worker_info.hpp"        // for WorkerInfo
#include "amdinfer/declarations.hpp"            // for RequestContainerPtr
#include "amdinfer/observation/metrics.hpp"     // for Metrics, MetricCounterIDs
#include "amdinfer/observation/tracing.hpp"     // for Trace
#include "amdinfer/util/queue.hpp"              // for BlockingConcurrentQueue
#include "amdinfer/util/thread.hpp"             // for setThreadName

// IWYU pragma: no_forward_declare amdinfer::Buffer

namespace amdinfer {

void HardBatcher::doRun(const std::vector<MemoryAllocators>& allocators) {
  auto thread_name = "batch" + this->getName();
  util::setThreadName(thread_name);
  RequestContainerPtr req;
  bool run = true;

  while (run) {
    auto batch = std::make_unique<Batch>();
    size_t batch_size = 0;

    std::vector<BufferPtr> input_buffers;
    std::vector<size_t> input_offset;
    std::vector<size_t> output_offset;

    bool first_request = true;

    do {
      this->input_queue_->wait_dequeue(req);

      if (req == nullptr) {
        run = false;
        break;
      }

#ifdef AMDINFER_ENABLE_TRACING
      auto& trace = req->trace;
      trace->startSpan("hard_batcher");
#endif

#ifdef AMDINFER_ENABLE_METRICS
      Metrics::getInstance().incrementCounter(
        MetricCounterIDs::PipelineIngressBatcher);
#endif

      auto request = req->request;
      const auto& inputs = request->getInputs();
      auto input_size = inputs.size();
      if (input_size == 0) {
        request->runCallbackError("Input size is zero");
        continue;
      }

      if (first_request) {
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

      auto raw_inputs = batch->getRawInputBuffers();
      auto raw_outputs = batch->getRawOutputBuffers();

      auto old_input_offset = input_offset;
      for (auto i = 0U; i < input_size; ++i) {
        const auto& input = inputs[i];
        auto* raw_input = raw_inputs[i];
        auto& offset = input_offset[i];

        auto new_offset =
          raw_input->write(input.getData(), offset,
                           input.getSize() * input.getDatatype().size());
        pool_->put(MemoryAllocators::Cpu, input.getData());
        request->setInputTensorData(i, raw_input->data(offset));
        offset = new_offset;
      }

      batch->addRequest(request);
      batch_size++;
#ifdef AMDINFER_ENABLE_TRACING
      trace->endSpan();
      batch->addTrace(std::move(trace));
#endif
#ifdef AMDINFER_ENABLE_METRICS
      batch->addTime(req->start_time);
#endif
      first_request = false;
    } while (batch_size % this->batch_size_ != 0);

    if (!batch->empty()) {
      this->output_queue_->enqueue(std::move(batch));
#ifdef AMDINFER_ENABLE_METRICS
      Metrics::getInstance().incrementCounter(
        MetricCounterIDs::PipelineEgressBatcher);
#endif
    }
  }
}

}  // namespace amdinfer
