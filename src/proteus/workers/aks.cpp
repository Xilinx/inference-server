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
 * @brief Implements the Aks worker
 */

#include <aks/AksSysManagerExt.h>  // for SysManagerExt
#include <aks/AksTensorBuffer.h>   // for AksTensorBuffer

#include <cstddef>                 // for size_t, byte
#include <cstdint>                 // for int32_t
#include <future>                  // for future
#include <memory>                  // for unique_ptr, allocator
#include <string>                  // for string, basic_string
#include <thread>                  // for thread
#include <utility>                 // for move, pair
#include <vart/tensor_buffer.hpp>  // for TensorBuffer
#include <vector>                  // for vector
#include <xir/tensor/tensor.hpp>   // for Tensor
#include <xir/util/data_type.hpp>  // for create_data_type

#include "proteus/batching/batcher.hpp"       // for Batch, BatchPtrQueue
#include "proteus/buffers/vector_buffer.hpp"  // for VectorBuffer
#include "proteus/build_options.hpp"          // for PROTEUS_ENABLE_TRACING
#include "proteus/core/data_types.hpp"        // for DataType, DataType::FP32
#include "proteus/core/predict_api.hpp"       // for InferenceRequest, Infere...
#include "proteus/helpers/declarations.hpp"   // for BufferPtr, InferenceResp...
#include "proteus/helpers/parse_env.hpp"      // for autoExpandEnvironmentVar...
#include "proteus/helpers/thread.hpp"         // for setThreadName
#include "proteus/observation/logging.hpp"    // for SPDLOG_LOGGER_INFO, SPDL...
#include "proteus/observation/metrics.hpp"    // for Metrics
#include "proteus/observation/tracing.hpp"    // for startFollowSpan, SpanPtr
#include "proteus/workers/worker.hpp"         // for Worker

namespace AKS {
class AIGraph;
}  // namespace AKS

namespace proteus {

using types::DataType;

namespace workers {

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

 private:
  void doInit(RequestParameters* parameters) override;
  size_t doAllocate(size_t num) override;
  void doAcquire(RequestParameters* parameters) override;
  void doRun(BatchPtrQueue* input_queue) override;
  void doRelease() override;
  void doDeallocate() override;
  void doDestroy() override;

  /// the AKS system manager
  AKS::SysManagerExt* sysMan_ = nullptr;
  /// the corresponding graph to the name
  AKS::AIGraph* graph_ = nullptr;
};

std::thread Aks::spawn(BatchPtrQueue* input_queue) {
  return std::thread(&Aks::run, this, input_queue);
}

void Aks::doInit(RequestParameters* parameters) {
  constexpr auto kBatchSize = 1;

  /// Get AKS System Manager instance
  this->sysMan_ = AKS::SysManagerExt::getGlobal();

  auto batch_size = kBatchSize;
  if (parameters->has("batch_size")) {
    batch_size = parameters->get<int32_t>("batch_size");
  }
  this->batch_size_ = batch_size;
}

size_t Aks::doAllocate(size_t num) {
  constexpr auto kBufferNum = 10U;
  size_t buffer_num =
    static_cast<int>(num) == kNumBufferAuto ? kBufferNum : num;
  VectorBuffer::allocate(this->input_buffers_, kBufferNum,
                         1 * this->batch_size_, DataType::FP32);
  VectorBuffer::allocate(this->output_buffers_, kBufferNum,
                         1 * this->batch_size_, DataType::FP32);
  return buffer_num;
}

void Aks::doAcquire(RequestParameters* parameters) {
  auto kPath = std::string("${AKS_ROOT}/graph_zoo/graph_adder.json");

  auto path = kPath;
  if (parameters->has("aks_graph")) {
    path = parameters->get<std::string>("aks_graph");
  }
  autoExpandEnvironmentVariables(path);
  this->sysMan_->loadGraphs(path);

  std::string graph_name = "graph_adder";
  this->graph_ = sysMan_->getGraph(graph_name);
  if (this->graph_ == nullptr) {
    throw std::runtime_error("AKS graph " + graph_name + " not found");
  }

  this->metadata_.addInputTensor("input", types::DataType::FP32,
                                 {this->batch_size_, 1});
  this->metadata_.addOutputTensor("output", types::DataType::FP32,
                                  {this->batch_size_, 1});
  this->metadata_.setName(graph_name);
}

void Aks::doRun(BatchPtrQueue* input_queue) {
  std::shared_ptr<InferenceRequest> req;
  std::unique_ptr<Batch> batch;
  setThreadName("Aks");

  while (true) {
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }
    SPDLOG_LOGGER_INFO(this->logger_, "Got request in aks");
    for (unsigned int j = 0; j < batch->requests->size(); j++) {
      auto& req = batch->requests->at(j);
#ifdef PROTEUS_ENABLE_TRACING
      auto span = startFollowSpan(batch->spans.at(j).get(), "aks");
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

        auto* inDataPtr = reinterpret_cast<float*>(v[0]->data().first);
        inDataPtr[0] = value;

        std::future<std::vector<std::unique_ptr<vart::TensorBuffer>>>
          futureObj =
            this->sysMan_->enqueueJob(this->graph_, "", std::move(v), nullptr);

        auto outDD = futureObj.get();

        value = (reinterpret_cast<float*>(outDD[0]->data().first))[0];

        InferenceResponseOutput output;
        output.setDatatype(types::DataType::FP32);
        output.setName("aks");
        output.setShape({1});
        auto buffer = std::make_shared<std::vector<float>>();
        buffer->resize(1);
        (*buffer)[0] = value;
        auto my_data_cast = std::reinterpret_pointer_cast<std::byte>(buffer);
        output.setData(std::move(my_data_cast));
        resp.addOutput(output);
      }

#ifdef PROTEUS_ENABLE_METRICS
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - batch->start_times[j]);
      Metrics::getInstance().observeSummary(MetricSummaryIDs::kRequestLatency,
                                            duration.count());
#endif

      req->runCallbackOnce(resp);
    }
    this->returnBuffers(std::move(batch->input_buffers),
                        std::move(batch->output_buffers));
    SPDLOG_LOGGER_DEBUG(this->logger_, "Returned buffers");
  }
  SPDLOG_LOGGER_INFO(this->logger_, "Aks ending");
}

void Aks::doRelease() {}
void Aks::doDeallocate() {}
void Aks::doDestroy() {}

}  // namespace workers

}  // namespace proteus

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
proteus::workers::Worker* getWorker() {
  return new proteus::workers::Aks("AKS", "AKS");
}
}  // extern C
