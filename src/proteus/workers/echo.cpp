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
 * @brief Implements the Echo worker
 */

#include <cstddef>  // for size_t, byte
#include <cstdint>  // for uint32_t, int32_t
#include <memory>   // for unique_ptr, allocator
#include <string>   // for string
#include <thread>   // for thread
#include <utility>  // for move
#include <vector>   // for vector

#include "proteus/batching/hard.hpp"          // for HardBatcher
#include "proteus/buffers/vector_buffer.hpp"  // for VectorBuffer
#include "proteus/build_options.hpp"          // for PROTEUS_ENABLE_TRACING
#include "proteus/core/data_types.hpp"        // for DataType, DataType::UINT32
#include "proteus/core/predict_api.hpp"       // for InferenceRequest, Infere...
#include "proteus/helpers/declarations.hpp"   // for BufferPtr, InferenceResp...
#include "proteus/helpers/thread.hpp"         // for setThreadName
#include "proteus/observation/logging.hpp"    // for SPDLOG_LOGGER_INFO, SPDL...
#include "proteus/observation/metrics.hpp"    // for Metrics
#include "proteus/observation/tracing.hpp"    // for startFollowSpan, SpanPtr
#include "proteus/workers/worker.hpp"         // for Worker

namespace proteus {

using types::DataType;

namespace workers {

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
  std::vector<std::unique_ptr<Batcher>> makeBatcher(int num = 1) override {
    return this->makeBatcher<HardBatcher>(num);
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
                         1 * this->batch_size_, DataType::UINT32);
  VectorBuffer::allocate(this->output_buffers_, buffer_num,
                         1 * this->batch_size_, DataType::UINT32);
  return buffer_num;
}

void Echo::doAcquire(RequestParameters* parameters) {
  (void)parameters;  // suppress unused variable warning

  this->metadata_.addInputTensor("input", types::DataType::UINT32, {1});
  this->metadata_.addOutputTensor("output", types::DataType::UINT32, {1});
}

void Echo::doRun(BatchPtrQueue* input_queue) {
  std::shared_ptr<InferenceRequest> req;
  std::unique_ptr<Batch> batch;
  setThreadName("Echo");

  while (true) {
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }
    SPDLOG_LOGGER_INFO(this->logger_, "Got request in echo");
#ifdef PROTEUS_ENABLE_TRACING
    auto span = startFollowSpan(batch->span.get(), "echo");
#endif
#ifdef PROTEUS_ENABLE_METRICS
    Metrics::getInstance().incrementCounter(
      MetricIDs::kCounterPipelineIngressWorker);
#endif
    for (auto& req : *(batch->requests)) {
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

        // this is my operation: add one to the read argument
        value++;

        // output_buffer->write(value);

        InferenceResponseOutput output;
        output.setDatatype(types::DataType::UINT32);
        std::string output_name = outputs[i].getName();
        if (output_name.empty()) {
          output.setName(inputs[i].getName());
        } else {
          output.setName(output_name);
        }
        output.setShape({1});
        auto buffer = std::make_shared<std::vector<uint32_t>>();
        buffer->resize(1);
        (*buffer)[0] = value;
        auto my_data_cast = std::reinterpret_pointer_cast<std::byte>(buffer);
        output.setData(std::move(my_data_cast));
        resp.addOutput(output);
      }

      // respond back to the client
      req->getCallback()(resp);
#ifdef PROTEUS_ENABLE_METRICS
      Metrics::getInstance().incrementCounter(
        MetricIDs::kCounterPipelineEgressWorker);
#endif
    }
    this->returnBuffers(std::move(batch->input_buffers),
                        std::move(batch->output_buffers));
    SPDLOG_LOGGER_DEBUG(this->logger_, "Returned buffers");
  }
  SPDLOG_LOGGER_INFO(this->logger_, "Echo ending");
}

void Echo::doRelease() {}
void Echo::doDeallocate() {}
void Echo::doDestroy() {}

}  // namespace workers

}  // namespace proteus

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
proteus::workers::Worker* getWorker() {
  return new proteus::workers::Echo("echo", "cpu");
}
}  // extern C
