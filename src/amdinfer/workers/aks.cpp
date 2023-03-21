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
 * @brief Implements the Aks worker
 */

#include <aks/AksSysManagerExt.h>  // for SysManagerExt
#include <aks/AksTensorBuffer.h>   // for AksTensorBuffer

#include <algorithm>               // for max
#include <cstddef>                 // for size_t, byte
#include <cstdint>                 // for int32_t
#include <cstring>                 // for memcpy
#include <future>                  // for future
#include <memory>                  // for unique_ptr, allocator
#include <ratio>                   // for micro
#include <string>                  // for string, operator+, char...
#include <thread>                  // for thread
#include <utility>                 // for move, pair
#include <vart/tensor_buffer.hpp>  // for TensorBuffer
#include <vector>                  // for vector
#include <xir/tensor/tensor.hpp>   // for Tensor
#include <xir/util/data_type.hpp>  // for create_data_type

#include "amdinfer/batching/batcher.hpp"  // for BatchPtr, Batch, BatchP...
#include "amdinfer/build_options.hpp"     // for AMDINFER_ENABLE_TRACING
#include "amdinfer/core/data_types.hpp"   // for DataType, DataType::Fp32
#include "amdinfer/core/exceptions.hpp"   // for external_error
#include "amdinfer/core/inference_request.hpp"   // for InferenceRequest
#include "amdinfer/core/inference_response.hpp"  // for InferenceResponse
#include "amdinfer/core/parameters.hpp"          // for ParameterMap
#include "amdinfer/declarations.hpp"         // for BufferPtrs, InferenceRe...
#include "amdinfer/observation/logging.hpp"  // for Logger
#include "amdinfer/observation/metrics.hpp"  // for Metrics, MetricSummaryIDs
#include "amdinfer/observation/tracing.hpp"  // for Trace
#include "amdinfer/util/parse_env.hpp"       // for autoExpandEnvironmentVa...
#include "amdinfer/util/thread.hpp"          // for setThreadName
#include "amdinfer/util/timer.hpp"           // for Timer
#include "amdinfer/workers/worker.hpp"       // for Worker, kNumBufferAuto

namespace AKS {  // NOLINT(readability-identifier-naming)
class AIGraph;
}  // namespace AKS

namespace amdinfer::workers {

/**
 * @brief The Aks worker is a simple worker that accepts a single uint32_t
 * argument and adds 1 to it and returns. It accepts multiple input tensors and
 * returns the corresponding number of output tensors.
 *
 */
class Aks : public Worker {
 public:
  using Worker::Worker;
  std::thread spawn(BatchPtrQueue* input_queue) override;
  [[nodiscard]] std::vector<MemoryAllocators> getAllocators() const override;

 private:
  void doInit(ParameterMap* parameters) override;
  void doAcquire(ParameterMap* parameters) override;
  void doRun(BatchPtrQueue* input_queue) override;
  void doRelease() override;
  void doDestroy() override;

  /// the AKS system manager
  AKS::SysManagerExt* sys_manager_ = nullptr;
  /// the corresponding graph to the name
  AKS::AIGraph* graph_ = nullptr;
};

std::thread Aks::spawn(BatchPtrQueue* input_queue) {
  return std::thread(&Aks::run, this, input_queue);
}

std::vector<MemoryAllocators> Aks::getAllocators() const {
  return {MemoryAllocators::Cpu};
}

void Aks::doInit(ParameterMap* parameters) {
  // arbitrarily set the default batch size to 1
  const int default_batch_size = 1;

  /// Get AKS System Manager instance
  this->sys_manager_ = AKS::SysManagerExt::getGlobal();

  auto batch_size = default_batch_size;
  if (parameters->has("batch_size")) {
    batch_size = parameters->get<int32_t>("batch_size");
  }
  this->batch_size_ = batch_size;
}

void Aks::doAcquire(ParameterMap* parameters) {
  std::string path{"${AKS_ROOT}/graph_zoo/graph_adder.json"};
  if (parameters->has("aks_graph")) {
    path = parameters->get<std::string>("aks_graph");
  }
  util::autoExpandEnvironmentVariables(path);
  this->sys_manager_->loadGraphs(path);

  std::string graph_name = "graph_adder";
  this->graph_ = sys_manager_->getGraph(graph_name);
  if (this->graph_ == nullptr) {
    throw external_error("AKS graph " + graph_name + " not found");
  }

  this->metadata_.addInputTensor("input", {this->batch_size_, 1},
                                 DataType::Fp32);
  this->metadata_.addOutputTensor("output", {this->batch_size_, 1},
                                  DataType::Fp32);
  this->metadata_.setName(graph_name);
}

void Aks::doRun(BatchPtrQueue* input_queue) {
  util::setThreadName("Aks");
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  while (true) {
    BatchPtr batch;
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }
    AMDINFER_LOG_INFO(logger, "Got request in aks");
    for (unsigned int j = 0; j < batch->size(); j++) {
      const auto& req = batch->getRequest(static_cast<int>(j));
#ifdef AMDINFER_ENABLE_TRACING
      const auto& trace = batch->getTrace(static_cast<int>(j));
      trace->startSpan("aks");
#endif
      InferenceResponse resp;
      resp.setID(req->getID());
      resp.setModel("aks");
      auto inputs = req->getInputs();
      auto outputs = req->getOutputs();

      for (auto& input : inputs) {
        auto* input_buffer = input.getData();

        float value = 0.0F;
        float* data = nullptr;
        data = static_cast<float*>(input_buffer);
        value = *data;

        std::vector<std::unique_ptr<vart::TensorBuffer>> v;
        v.emplace_back(std::make_unique<AKS::AksTensorBuffer>(
          xir::Tensor::create("aks-echo", {1}, xir::create_data_type<int>())));

        auto* data_in_ptr = reinterpret_cast<float*>(v[0]->data().first);
        data_in_ptr[0] = value;

        std::future<std::vector<std::unique_ptr<vart::TensorBuffer>>> future =
          this->sys_manager_->enqueueJob(this->graph_, "", std::move(v),
                                         nullptr);

        auto out_data_descriptor = future.get();

        value =
          (reinterpret_cast<float*>(out_data_descriptor[0]->data().first))[0];

        InferenceResponseOutput output;
        output.setDatatype(DataType::Fp32);
        output.setName("aks");
        output.setShape({1});
        std::vector<std::byte> buffer;
        buffer.resize(sizeof(float));
        memcpy(buffer.data(), &value, sizeof(float));
        output.setData(std::move(buffer));
        resp.addOutput(output);
      }

#ifdef AMDINFER_ENABLE_METRICS
      util::Timer timer{batch->getTime(j)};
      timer.stop();
      auto duration = timer.count<std::micro>();
      Metrics::getInstance().observeSummary(MetricSummaryIDs::RequestLatency,
                                            duration);
#endif
#ifdef AMDINFER_ENABLE_TRACING
      auto context = trace->propagate();
      resp.setContext(std::move(context));
#endif
      req->runCallbackOnce(resp);
    }
  }
  AMDINFER_LOG_INFO(logger, "Aks ending");
}

void Aks::doRelease() {}
void Aks::doDestroy() {}

}  // namespace amdinfer::workers

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
amdinfer::workers::Worker* getWorker() {
  return new amdinfer::workers::Aks("AKS", "AKS");
}
}  // extern C
