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
 * @brief Implements the Echo worker
 */

#include <chrono>     // for microseconds, duration_...
#include <cstddef>    // for size_t, byte
#include <cstdint>    // for uint32_t, int32_t
#include <exception>  // for exception
#include <memory>     // for unique_ptr, allocator
#include <string>     // for string
#include <thread>     // for thread
#include <utility>    // for move
#include <vector>     // for vector

#include "amdinfer/batching/hard.hpp"          // for HardBatcher
#include "amdinfer/buffers/vector_buffer.hpp"  // for VectorBuffer
#include "amdinfer/build_options.hpp"          // for AMDINFER_ENABLE_TRACING
#include "amdinfer/core/data_types.hpp"        // for DataType, DataType::Uint32
#include "amdinfer/core/predict_api.hpp"       // for InferenceRequest, Infer...
#include "amdinfer/declarations.hpp"           // for BufferPtr, InferenceRes...
#include "amdinfer/observation/logging.hpp"    // for Logger
#include "amdinfer/observation/metrics.hpp"    // for Metrics
#include "amdinfer/observation/tracing.hpp"    // for startFollowSpan, SpanPtr
#include "amdinfer/util/thread.hpp"            // for setThreadName
#include "amdinfer/workers/worker.hpp"         // for Worker

namespace amdinfer::workers {

/**
 * @brief The Echo worker is a simple worker that accepts a single uint32_t
 * argument and adds 1 to it and returns. It accepts multiple input tensors and
 * returns the corresponding number of output tensors.
 *
 */
class Echo : public Worker {
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

  // workers define what batcher implementation should be used for them.
  // if not explicitly defined here, a default value is used from worker.hpp.
  using Worker::makeBatcher;
  std::vector<std::unique_ptr<Batcher>> makeBatcher(
    int num, RequestParameters* parameters) override {
    return this->makeBatcher<HardBatcher>(num, parameters);
  };
};

std::thread Echo::spawn(BatchPtrQueue* input_queue) {
  return std::thread(&Echo::run, this, input_queue);
}

void Echo::doInit(RequestParameters* parameters) {
  constexpr auto kMaxBufferNum = 50;
  constexpr auto kBatchSize = 1;

  auto max_buffer_num = kMaxBufferNum;
  if (parameters->has("max_buffer_num")) {
    max_buffer_num = parameters->get<int32_t>("max_buffer_num");
  }
  this->max_buffer_num_ = max_buffer_num;

  auto batch_size = kBatchSize;
  if (parameters->has("batch_size")) {
    batch_size = parameters->get<int32_t>("batch_size");
  }
  this->batch_size_ = batch_size;
}

size_t Echo::doAllocate(size_t num) {
  constexpr auto kBufferNum = 10U;
  size_t buffer_num =
    static_cast<int>(num) == kNumBufferAuto ? kBufferNum : num;
  VectorBuffer::allocate(this->input_buffers_, buffer_num,
                         1 * this->batch_size_, DataType::Uint32);
  VectorBuffer::allocate(this->output_buffers_, buffer_num,
                         1 * this->batch_size_, DataType::Uint32);
  return buffer_num;
}

void Echo::doAcquire(RequestParameters* parameters) {
  (void)parameters;  // suppress unused variable warning

  this->metadata_.addInputTensor("input", DataType::Uint32, {1});
  this->metadata_.addOutputTensor("output", DataType::Uint32, {1});
}

void Echo::doRun(BatchPtrQueue* input_queue) {
  util::setThreadName("Echo");
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  while (true) {
    BatchPtr batch;
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }
    AMDINFER_LOG_INFO(logger, "Got request in echo");
#ifdef AMDINFER_ENABLE_METRICS
    Metrics::getInstance().incrementCounter(
      MetricCounterIDs::kPipelineIngressWorker);
#endif
    for (unsigned int j = 0; j < batch->size(); j++) {
      const auto& req = batch->getRequest(j);
#ifdef AMDINFER_ENABLE_TRACING
      const auto& trace = batch->getTrace(j);
      trace->startSpan("echo");
#endif
      InferenceResponse resp;
      resp.setID(req->getID());
      resp.setModel("echo");
      auto inputs = req->getInputs();
      auto outputs = req->getOutputs();
      for (unsigned int i = 0; i < inputs.size(); i++) {
        auto* input_buffer = inputs[i].getData();
        // std::byte* output_buffer = outputs[i].getData();
        // auto* input_buffer = dynamic_cast<VectorBuffer*>(input_ptr);
        // auto* output_buffer = dynamic_cast<VectorBuffer*>(output_ptr);

        uint32_t value = *static_cast<uint32_t*>(input_buffer);

        // this is my operation: add one to the read argument. While this can't
        // raise an exception, if exceptions can happen, they should be handled
        try {
          value++;
        } catch (const std::exception& e) {
          AMDINFER_LOG_ERROR(logger, e.what());
          req->runCallbackError("Something went wrong");
          continue;
        }

        // output_buffer->write(value);

        InferenceResponseOutput output;
        output.setDatatype(DataType::Uint32);
        std::string output_name = outputs[i].getName();
        if (output_name.empty()) {
          output.setName(inputs[i].getName());
        } else {
          output.setName(output_name);
        }
        output.setShape({1});
        std::vector<std::byte> buffer;
        buffer.resize(sizeof(uint32_t));
        memcpy(buffer.data(), &value, sizeof(uint32_t));
        output.setData(std::move(buffer));
        resp.addOutput(output);
      }

#ifdef AMDINFER_ENABLE_TRACING
      auto context = trace->propagate();
      resp.setContext(std::move(context));
#endif

      // respond back to the client
      req->runCallbackOnce(resp);
#ifdef AMDINFER_ENABLE_METRICS
      Metrics::getInstance().incrementCounter(
        MetricCounterIDs::kPipelineEgressWorker);
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - batch->getTime(j));
      Metrics::getInstance().observeSummary(MetricSummaryIDs::kRequestLatency,
                                            duration.count());
#endif
    }
  }
  AMDINFER_LOG_INFO(logger, "Echo ending");
}

void Echo::doRelease() {}
void Echo::doDeallocate() {}
void Echo::doDestroy() {}

}  // namespace amdinfer::workers

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
amdinfer::workers::Worker* getWorker() {
  return new amdinfer::workers::Echo("echo", "cpu");
}
}  // extern C
