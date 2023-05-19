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
 * @brief Implements the Responder worker
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

#include "amdinfer/batching/hard.hpp"    // for HardBatcher
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
#include "amdinfer/util/thread.hpp"          // for setThreadName
#include "amdinfer/util/timer.hpp"           // for Timer
#include "amdinfer/workers/worker.hpp"       // for Worker

namespace amdinfer::workers {

/**
 * @brief The Responder worker can run a compiled C++ "model".
 *
 */
class Responder : public SingleThreadedWorker {
 public:
  using SingleThreadedWorker::SingleThreadedWorker;
  [[nodiscard]] std::vector<MemoryAllocators> getAllocators() const override;

 private:
  void doInit(ParameterMap* parameters) override;
  void doAcquire(ParameterMap* parameters) override;
  BatchPtr doRun(Batch* batch, const MemoryPool* pool) override;
  void doRelease() override;
  void doDestroy() override;

  std::vector<Tensor> input_tensors_;
  std::vector<Tensor> output_tensors_;

  // workers define what batcher implementation should be used for them.
  // if not explicitly defined here, a default value is used from worker.hpp.
  using Worker::makeBatcher;
  std::vector<std::unique_ptr<Batcher>> makeBatcher(int num,
                                                    ParameterMap* parameters,
                                                    MemoryPool* pool) override {
    return this->makeBatcher<HardBatcher>(num, parameters, pool);
  };
};

std::vector<MemoryAllocators> Responder::getAllocators() const {
  return {MemoryAllocators::Cpu};
}

void Responder::doInit(ParameterMap* parameters) {
  constexpr auto kBatchSize = 1;

  auto batch_size = kBatchSize;
  if (parameters->has("batch_size")) {
    batch_size = parameters->get<int32_t>("batch_size");
  }
  this->batch_size_ = batch_size;
}

void Responder::doAcquire([[maybe_unused]] ParameterMap* parameters) {
  // TODO(varunsh): what should we do for metadata_?
}

BatchPtr Responder::doRun(Batch* batch,
                          [[maybe_unused]] const MemoryPool* pool) {
  const auto batch_size = batch->size();
  for (unsigned int j = 0; j < batch_size; j++) {
    const auto& req = batch->getRequest(j);

    InferenceResponse resp;
    resp.setID(req->getID());
    resp.setModel(batch->getModel(j));
    auto inputs = req->getInputs();
    auto outputs = req->getOutputs();
    for (unsigned int i = 0; i < inputs.size(); i++) {
      const auto& input = inputs[i];
      const auto* input_buffer = input.getData();

      InferenceResponseOutput output;
      output.setDatatype(input.getDatatype());
      std::string output_name;
      if (i < outputs.size()) {
        output_name = outputs[i].getName();
      }

      if (output_name.empty()) {
        output.setName(input.getName());
      } else {
        output.setName(output_name);
      }
      output.setShape(input.getShape());
      std::vector<std::byte> buffer;
      const auto size = input.getSize() * input.getDatatype().size();
      buffer.resize(size);
      memcpy(buffer.data(), input_buffer, size);
      output.setData(std::move(buffer));
      resp.addOutput(output);
    }

#ifdef AMDINFER_ENABLE_TRACING
    const auto& trace = batch->getTrace(j);
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
  // okay because ensembles disabled for this worker
  return nullptr;
}

void Responder::doRelease() {}
void Responder::doDestroy() {}

}  // namespace amdinfer::workers

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
amdinfer::workers::Worker* getWorker() {
  return new amdinfer::workers::Responder("responder", "CPU", false);
}
}  // extern C
