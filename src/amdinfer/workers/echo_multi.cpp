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
 * @brief Implements the EchoMulti worker
 */

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

#include "amdinfer/batching/hard.hpp"        // for HardBatcher
#include "amdinfer/build_options.hpp"        // for AMDINFER_ENABLE_TRACING
#include "amdinfer/core/data_types.hpp"      // for DataType, DataType::Uint32
#include "amdinfer/core/parameters.hpp"      // for ParameterMap
#include "amdinfer/core/predict_api.hpp"     // for InferenceRequest, Infer...
#include "amdinfer/declarations.hpp"         // for BufferPtr, InferenceRes...
#include "amdinfer/observation/logging.hpp"  // for Logger
#include "amdinfer/observation/metrics.hpp"  // for Metrics
#include "amdinfer/observation/tracing.hpp"  // for startFollowSpan, SpanPtr
#include "amdinfer/util/containers.hpp"      // for containerSum
#include "amdinfer/util/queue.hpp"           // for BufferPtrsQueue
#include "amdinfer/util/thread.hpp"          // for setThreadName
#include "amdinfer/util/timer.hpp"           // for Timer
#include "amdinfer/workers/worker.hpp"       // for Worker

namespace amdinfer {

const int kInputTensors = 2;
const std::array<int, kInputTensors> kInputLengths = {1, 2};
const int kOutputTensors = 3;
const std::array<int, kOutputTensors> kOutputLengths = {1, 4, 3};

namespace workers {

/**
 * @brief The EchoMulti worker is a simple worker that accepts two input tensors
 * and produces 3 output tensors as a test case for multi-input/output models.
 *
 */
class EchoMulti : public Worker {
 public:
  using Worker::Worker;
  std::thread spawn(BatchPtrQueue* input_queue) override;
  std::vector<MemoryAllocators> getAllocators() const override;

 private:
  void doInit(ParameterMap* parameters) override;
  void doAcquire(ParameterMap* parameters) override;
  void doRun(BatchPtrQueue* input_queue) override;
  void doRelease() override;
  void doDestroy() override;

  // workers define what batcher implementation should be used for them.
  // if not explicitly defined here, a default value is used from worker.hpp.
  using Worker::makeBatcher;
  std::vector<std::unique_ptr<Batcher>> makeBatcher(int num,
                                                    ParameterMap* parameters,
                                                    MemoryPool* pool) override {
    return this->makeBatcher<HardBatcher>(num, parameters, pool);
  };
};

std::thread EchoMulti::spawn(BatchPtrQueue* input_queue) {
  return std::thread(&EchoMulti::run, this, input_queue);
}

std::vector<MemoryAllocators> EchoMulti::getAllocators() const {
  return {MemoryAllocators::Cpu};
}

void EchoMulti::doInit(ParameterMap* parameters) {
  constexpr auto kBatchSize = 1;

  auto batch_size = kBatchSize;
  if (parameters->has("batch_size")) {
    batch_size = parameters->get<int32_t>("batch_size");
  }
  this->batch_size_ = batch_size;
}

void EchoMulti::doAcquire([[maybe_unused]] ParameterMap* parameters) {
  for (auto i = 0; i < kInputTensors; ++i) {
    this->metadata_.addInputTensor(
      "input" + std::to_string(i), DataType::Uint32,
      {static_cast<uint64_t>(kInputLengths.at(i))});
  }
  for (auto i = 0; i < kOutputTensors; ++i) {
    this->metadata_.addInputTensor(
      "input" + std::to_string(i), DataType::Uint32,
      {static_cast<uint64_t>(kOutputLengths.at(i))});
  }
}

void EchoMulti::doRun(BatchPtrQueue* input_queue) {
  util::setThreadName("EchoMulti");
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  while (true) {
    BatchPtr batch;
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }
    AMDINFER_LOG_INFO(logger, "Got request in echoMulti");
#ifdef AMDINFER_ENABLE_METRICS
    Metrics::getInstance().incrementCounter(
      MetricCounterIDs::PipelineIngressWorker);
#endif
    const auto batch_size = batch->size();
    for (unsigned int j = 0; j < batch_size; j++) {
      const auto& req = batch->getRequest(static_cast<int>(j));
#ifdef AMDINFER_ENABLE_TRACING
      const auto& trace = batch->getTrace(static_cast<int>(j));
      trace->startSpan("echoMulti");
#endif
      InferenceResponse resp;
      resp.setID(req->getID());
      resp.setModel("echoMulti");
      auto inputs = req->getInputs();
      auto outputs = req->getOutputs();

      std::vector<int> args;
      const auto input_num = util::containerSum(kInputLengths);
      assert(input_num != 0);
      args.reserve(input_num);
      for (auto i = 0; i < kInputTensors; ++i) {
        const auto* input_buffer = static_cast<uint32_t*>(inputs[i].getData());
        for (auto k = 0; k < kInputLengths.at(i); ++k) {
          args.push_back(input_buffer[k]);
        }
      }

      std::vector<int> output_args;
      auto input_index = 0;
      auto offset = 0;
      output_args.reserve(util::containerSum(kOutputLengths));
      for (auto i = 0; i < kOutputTensors; ++i) {
        for (auto k = 0; k < kOutputLengths.at(i); ++k) {
          output_args.push_back(args[input_index]);
          input_index = (input_index + 1) % input_num;
        }

        InferenceResponseOutput output;
        output.setDatatype(DataType::Uint32);
        std::string output_name;
        if (static_cast<size_t>(i) < outputs.size()) {
          output_name = outputs[i].getName();
        }

        if (output_name.empty()) {
          output.setName(inputs[0].getName());
        } else {
          output.setName(output_name);
        }
        output.setShape({static_cast<uint64_t>(kOutputLengths.at(i))});
        std::vector<std::byte> buffer;
        buffer.resize(kOutputLengths.at(i) * sizeof(uint32_t));
        memcpy(buffer.data(), &(output_args[offset]),
               kOutputLengths.at(i) * sizeof(uint32_t));
        output.setData(std::move(buffer));
        resp.addOutput(output);
        offset += kOutputLengths.at(i);
      }

#ifdef AMDINFER_ENABLE_TRACING
      auto context = trace->propagate();
      resp.setContext(std::move(context));
#endif

      // respond back to the client
      req->runCallbackOnce(resp);
#ifdef AMDINFER_ENABLE_METRICS
      Metrics::getInstance().incrementCounter(
        MetricCounterIDs::PipelineEgressWorker);
      util::Timer timer{batch->getTime(j)};
      timer.stop();
      auto duration = timer.count<std::micro>();
      Metrics::getInstance().observeSummary(MetricSummaryIDs::RequestLatency,
                                            duration);
#endif
    }
    this->returnInputBuffers(std::move(batch));
  }
  AMDINFER_LOG_INFO(logger, "EchoMulti ending");
}

void EchoMulti::doRelease() {}
void EchoMulti::doDestroy() {}

}  // namespace workers

}  // namespace amdinfer

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
amdinfer::workers::Worker* getWorker() {
  return new amdinfer::workers::EchoMulti("echoMulti", "cpu");
}
}  // extern C
