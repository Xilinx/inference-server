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
 * @brief Implements the XModel worker
 */

#include <cxxabi.h>  // for __forced_unwind

#include <algorithm>                    // for copy, copy_backward
#include <atomic>                       // for atomic_int32_t
#include <chrono>                       // for microseconds, dura...
#include <cstddef>                      // for size_t, byte
#include <cstdint>                      // for uint64_t, uint32_t
#include <cstdlib>                      // for getenv
#include <cstring>                      // for memcpy
#include <ext/alloc_traits.h>           // for __alloc_traits<>::...
#include <functional>                   // for multiplies
#include <memory>                       // for unique_ptr, allocator
#include <numeric>                      // for accumulate
#include <queue>                        // for queue
#include <string>                       // for string, operator!=
#include <thread>                       // for thread, sleep_for
#include <utility>                      // for pair, move
#include <vart/runner.hpp>              // for Runner
#include <vart/runner_ext.hpp>          // for RunnerExt
#include <vart/tensor_buffer.hpp>       // for TensorBuffer
#include <vector>                       // for vector
#include <vitis/ai/target_factory.hpp>  // for target_factory
#include <xir/graph/graph.hpp>          // for Graph
#include <xir/graph/subgraph.hpp>       // for Subgraph
#include <xir/tensor/tensor.hpp>        // for Tensor

#include "amdinfer/batching/batcher.hpp"            // for BatchPtr, Batch
#include "amdinfer/buffers/buffer.hpp"              // for Buffer
#include "amdinfer/buffers/vart_tensor_buffer.hpp"  // for VartTensorBuffer
#include "amdinfer/build_options.hpp"               // for AMDINFER_ENABLE_ME...
#include "amdinfer/core/data_types_internal.hpp"    // for mapXirType, DataType
#include "amdinfer/core/predict_api.hpp"            // for InferenceResponse
#include "amdinfer/declarations.hpp"                // for BufferPtrs, Infere...
#include "amdinfer/observation/observer.hpp"        // for Loggers, Metrics...
#include "amdinfer/util/ctpl.h"                     // for thread_pool
#include "amdinfer/util/parse_env.hpp"              // for autoExpandEnvironm...
#include "amdinfer/util/queue.hpp"                  // for BufferPtrsQueue
#include "amdinfer/util/string.hpp"                 // for endsWith
#include "amdinfer/util/thread.hpp"                 // for setThreadName
#include "amdinfer/workers/worker.hpp"              // for Worker, kNumBuffer...

uint64_t reduce_mult(std::vector<uint64_t>& v) {
  return std::accumulate(v.begin(), v.end(), 1, std::multiplies<>());
}

namespace amdinfer {

namespace workers {

/**
 * @brief The XModel worker accepts a path to an XModel from the user and runs
 * the DPU subgraph associated with the XModel. The incoming requests are sent
 * to the FPGA using the Vitis AI runtime libraries.
 *
 */
class XModel : public Worker {
 public:
  XModel() : Worker("XModel", "XModel") {}
  std::thread spawn(BatchPtrQueue* input_queue) override;

 private:
  void doInit(RequestParameters* parameters) override;
  size_t doAllocate(size_t num) override;
  void doAcquire(RequestParameters* parameters) override;
  void doRun(BatchPtrQueue* input_queue) override;
  void doRelease() override;
  void doDeallocate() override;
  void doDestroy() override;

  vart::RunnerExt* getRunner();

  std::unique_ptr<xir::Graph> graph_;
  const xir::Subgraph* subgraph_ = nullptr;
  std::string kernel_;
  std::unique_ptr<vart::Runner> runner_;
  std::vector<DataType> output_type_;
  std::vector<uint32_t> output_size_;
  ctpl::thread_pool pool_;
};

std::thread XModel::spawn(BatchPtrQueue* input_queue) {
  return std::thread(&XModel::run, this, input_queue);
}

vart::RunnerExt* XModel::getRunner() {
  return dynamic_cast<vart::RunnerExt*>(this->runner_.get());
}

void XModel::doInit(RequestParameters* parameters) {
  constexpr auto kMaxBufferNum = 50;
  const auto* aks_xmodel_root = std::getenv("AKS_XMODEL_ROOT");
  if (aks_xmodel_root == nullptr) {
    throw environment_not_set_error("AKS_XMODEL_ROOT not set");
  }
  const auto kPath =
    std::string{aks_xmodel_root} +
    "/artifacts/u200_u250/resnet_v1_50_tf/resnet_v1_50_tf.xmodel";

  auto max_buffer_num = kMaxBufferNum;
  if (parameters->has("max_buffer_num")) {
    max_buffer_num = parameters->get<int32_t>("max_buffer_num");
  }
  this->max_buffer_num_ = max_buffer_num;

  auto path = kPath;
  if (parameters->has("model")) {
    path = parameters->get<std::string>("model");
  }
  util::autoExpandEnvironmentVariables(path);
  graph_ = xir::Graph::deserialize(path);

  auto subgraphs = graph_->get_root_subgraph()->children_topological_sort();
  std::vector<const xir::Subgraph*> dpu_graphs;
  for (const auto* c : subgraphs) {
    // CHECK(c->has_attr("device"));
    auto device = c->get_attr<std::string>("device");
    if (device == "DPU") {
      dpu_graphs.emplace_back(c);
    }
  }
  // TODO(varunsh): we want to eventually support arbitrary numbers of dpu
  // graphs but may need model chaining for that
  if (dpu_graphs.size() > 1) {
    throw invalid_argument("Unsupported XModel with more than 1 DPU subgraph");
  }

  this->subgraph_ = dpu_graphs[0];
  if (this->subgraph_->has_attr("dpu_fingerprint")) {
    const auto fingerprint =
      this->subgraph_->get_attr<std::uint64_t>("dpu_fingerprint");
    this->kernel_ = vitis::ai::target_factory()->create(fingerprint).type();
  } else {
    this->kernel_ = this->subgraph_->get_attr<std::string>("kernel");
  }
}

size_t XModel::doAllocate(size_t num) {
  constexpr auto kBufferNum = 10U;
  size_t buffer_num =
    static_cast<int>(num) == kNumBufferAuto ? kBufferNum : num;

  for (size_t i = 0; i < buffer_num; i++) {
    BufferPtrs vec;
    auto input_tensors = subgraph_->get_sorted_input_tensors();
    if (input_tensors.size() > 1) {
      // rt-engine/vart does not support more than one input tensor
      throw(
        invalid_argument("Unsupported XModel with more than one input tensor"));
    }
    for (const auto* tensor : input_tensors) {
      auto input_shape = tensor->get_shape();
      auto input_type = tensor->get_data_type();
      vec.emplace_back(std::make_unique<VartTensorBuffer>(
        tensor->get_name(), input_shape, input_type));
    }
    this->input_buffers_->enqueue(std::move(vec));
  }
  for (size_t i = 0; i < buffer_num; i++) {
    BufferPtrs vec;
    auto output_tensors = subgraph_->get_sorted_output_tensors();
    for (const auto* tensor : output_tensors) {
      auto input_shape = tensor->get_shape();
      auto input_type = tensor->get_data_type();
      vec.emplace_back(std::make_unique<VartTensorBuffer>(
        tensor->get_name(), input_shape, input_type));
    }
    this->output_buffers_->enqueue(std::move(vec));
  }
  return buffer_num;
}

void XModel::doAcquire(RequestParameters* parameters) {
  constexpr auto kThreads = 3;

  auto threads = kThreads;
  if (parameters->has("threads")) {
    threads = parameters->get<int32_t>("threads");
  }
  this->pool_.resize(threads);

  runner_ = vart::Runner::create_runner(this->subgraph_, "run");
  auto input_tensors = runner_->get_input_tensors();
  // assuming only one tensor as in doInit()
  auto input_shape = input_tensors[0]->get_shape();
  auto input_type = mapXirToType(input_tensors[0]->get_data_type());
  this->batch_size_ = input_shape[0];
  this->metadata_.addInputTensor("input", input_type, input_shape);

  auto output_tensors = runner_->get_output_tensors();
  for (const auto* tensor : output_tensors) {
    auto output_shape = tensor->get_shape();
    output_type_.emplace_back(mapXirToType(tensor->get_data_type()));
    // +1 to skip the batch size
    output_size_.emplace_back(std::accumulate(
      output_shape.begin() + 1, output_shape.end(), 1, std::multiplies<>()));
    // TODO(varunsh): what should we return here?
    this->metadata_.addOutputTensor("output", output_type_.back(),
                                    output_shape);
  }
}

void XModel::doRun(BatchPtrQueue* input_queue) {
  std::atomic_int32_t pool_size = 0;
  const int max_pool_size = this->pool_.size() * 4;  // 4 is arbitrary
  util::setThreadName("XModel");
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  while (true) {
    BatchPtr batch;
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }
    AMDINFER_LOG_INFO(
      logger, "Got request in xmodel: " + std::to_string(batch->size()));
#ifdef AMDINFER_ENABLE_METRICS
    Metrics::getInstance().incrementCounter(
      MetricCounterIDs::kPipelineIngressWorker);
#endif
    pool_size++;
    if (pool_size > max_pool_size) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    this->pool_.push([this, batch = std::move(batch), &pool_size](int id) {
      (void)id;  // suppress unused variable warning
#ifdef AMDINFER_ENABLE_TRACING
      for (unsigned int j = 0; j < batch->size(); j++) {
        auto& trace = batch->getTrace(j);
        trace->startSpan("xmodel");
      }
#endif

      std::queue<std::pair<uint32_t, int>> futures;
      std::vector<vart::TensorBuffer*> inputsPtr;
      std::vector<vart::TensorBuffer*> outputsPtr;

      auto& input_buffers = batch->getInputBuffers();
      inputsPtr.reserve(input_buffers.size());
      auto& output_buffers = batch->getOutputBuffers();
      outputsPtr.reserve(output_buffers.size());

      for (const auto& buffer : input_buffers) {
        logTraceBuffer(getLogger(), buffer->data());
      }

      for (const auto& buffer : input_buffers) {
        auto* vart = dynamic_cast<VartTensorBuffer*>(buffer.get());
        inputsPtr.emplace_back(vart->getTensorBuffer());
      }
      for (const auto& buffer : output_buffers) {
        auto* vart = dynamic_cast<VartTensorBuffer*>(buffer.get());
        outputsPtr.emplace_back(vart->getTensorBuffer());
      }

      for (auto* input : inputsPtr) {
        const auto* tensor = input->get_tensor();
        input->sync_for_write(
          0, tensor->get_element_num() / (tensor->get_shape())[0]);
      }

      futures.push(getRunner()->execute_async(inputsPtr, outputsPtr));

      std::vector<InferenceResponse> responses;
      responses.reserve(batch->size());

      for (const auto& req : *batch) {
        auto& resp = responses.emplace_back();
        resp.setID(req->getID());
        resp.setModel("xmodel");
      }

      while (!futures.empty()) {
        auto job_id = futures.front();
        futures.pop();
        getRunner()->wait(job_id.first, -1);
      }

      for (auto* output : outputsPtr) {
        const auto* tensor = output->get_tensor();
        output->sync_for_read(
          0, tensor->get_element_num() / (tensor->get_shape())[0]);
      }

      const auto num_batches = batch->size();
      for (unsigned int k = 0; k < num_batches; k++) {
        const auto& req = batch->getRequest(k);
        auto inputs = req->getInputs();
        auto outputs = req->getOutputs();
        auto& resp = responses[k];

        const auto num_outputs = outputs.size();
        for (unsigned int i = 0; i < num_outputs; i++) {
          auto* output_index = output_buffers[i]->data();
          InferenceResponseOutput output;
          auto output_tensors = getRunner()->get_output_tensors();
          auto output_shape = output_tensors[i]->get_shape();
          std::vector<uint64_t> new_shape;
          new_shape.reserve(output_shape.size() - 1);
          for (size_t j = 0; j < output_shape.size() - 1; j++) {
            new_shape.push_back(output_shape[j + 1]);
          }
          output.setShape(new_shape);

          output.setDatatype(this->output_type_[i]);

          std::vector<std::byte> buffer;
          buffer.resize(this->output_size_[i] * sizeof(uint8_t));
          memcpy(buffer.data(),
                 reinterpret_cast<int8_t*>(output_index) +
                   (i * this->output_size_[i]),
                 this->output_size_[i] * sizeof(uint8_t));
          output.setData(std::move(buffer));

          std::string output_name = outputs[i].getName();

          if (output_name.empty()) {
            output.setName(inputs[0].getName());
          } else {
            output.setName(output_name);
          }

          resp.addOutput(output);
        }

#ifdef AMDINFER_ENABLE_TRACING
        auto context = batch->getTrace(k)->propagate();
        resp.setContext(std::move(context));
#endif

        req->runCallbackOnce(resp);
#ifdef AMDINFER_ENABLE_METRICS
        Metrics::getInstance().incrementCounter(
          MetricCounterIDs::kPipelineEgressWorker);
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::high_resolution_clock::now() - batch->getTime(k));
        Metrics::getInstance().observeSummary(MetricSummaryIDs::kRequestLatency,
                                              duration.count());
#endif
      }
      pool_size--;
    });
  }
  AMDINFER_LOG_INFO(logger, "XModel ending");
}

void XModel::doRelease() {}
void XModel::doDeallocate() { this->pool_.stop(true); }
void XModel::doDestroy() {}

}  // namespace workers

}  // namespace amdinfer

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
amdinfer::workers::Worker* getWorker() {
  return new amdinfer::workers::XModel();
}
}  // extern C
