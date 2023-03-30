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
#include <cstddef>                      // for size_t, byte
#include <cstdint>                      // for uint64_t, uint32_t
#include <cstdlib>                      // for getenv
#include <cstring>                      // for memcpy
#include <ext/alloc_traits.h>           // for __alloc_traits<>::...
#include <memory>                       // for unique_ptr, allocator
#include <queue>                        // for queue
#include <ratio>                        // for micro
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

#include "amdinfer/batching/batcher.hpp"          // for BatchPtr, Batch
#include "amdinfer/buffers/buffer.hpp"            // for Buffer
#include "amdinfer/buffers/vart_tensor.hpp"       // for VartTensorBuffer
#include "amdinfer/build_options.hpp"             // for AMDINFER_ENABLE_ME...
#include "amdinfer/core/data_types.hpp"           // for DataType
#include "amdinfer/core/data_types_internal.hpp"  // for mapXirToType
#include "amdinfer/core/exceptions.hpp"           // for invalid_argument
#include "amdinfer/core/inference_request.hpp"    // for InferenceRequest
#include "amdinfer/core/inference_response.hpp"   // for InferenceResponse
#include "amdinfer/core/parameters.hpp"           // for ParameterMap
#include "amdinfer/declarations.hpp"              // for BufferPtrs, Infere...
#include "amdinfer/observation/observer.hpp"      // for Loggers, Metrics...
#include "amdinfer/util/containers.hpp"           // for containerProduct
#include "amdinfer/util/memory.hpp"               // for copy
#include "amdinfer/util/parse_env.hpp"            // for autoExpandEnvironm...
#include "amdinfer/util/queue.hpp"                // for BufferPtrsQueue
#include "amdinfer/util/thread.hpp"               // for setThreadName
#include "amdinfer/util/timer.hpp"                // for Timer
#include "amdinfer/workers/worker.hpp"            // for Worker, kNumBuffer...

namespace amdinfer::workers {

/**
 * @brief The XModel worker accepts a path to an XModel from the user and runs
 * the DPU subgraph associated with the XModel. The incoming requests are sent
 * to the FPGA using the Vitis AI runtime libraries.
 *
 */
class XModel : public MultiThreadedWorker {
 public:
  using MultiThreadedWorker::MultiThreadedWorker;
  [[nodiscard]] std::vector<MemoryAllocators> getAllocators() const override;

 private:
  void doInit(ParameterMap* parameters) override;
  void doAcquire(ParameterMap* parameters) override;
  BatchPtr doRun(Batch* batch, const MemoryPool* pool) override;
  void doRelease() override;
  void doDestroy() override;

  vart::RunnerExt* getRunner();

  std::unique_ptr<xir::Graph> graph_;
  const xir::Subgraph* subgraph_ = nullptr;
  std::string kernel_;
  std::unique_ptr<vart::Runner> runner_;
  std::vector<DataType> output_type_;
  std::vector<uint32_t> output_size_;
};

std::vector<MemoryAllocators> XModel::getAllocators() const {
  return {MemoryAllocators::VartTensor};
}

vart::RunnerExt* XModel::getRunner() {
  return dynamic_cast<vart::RunnerExt*>(this->runner_.get());
}

void XModel::doInit(ParameterMap* parameters) {
  const auto* aks_xmodel_root = std::getenv("AKS_XMODEL_ROOT");
  if (aks_xmodel_root == nullptr) {
    throw environment_not_set_error("AKS_XMODEL_ROOT not set");
  }
  const auto default_path =
    std::string{aks_xmodel_root} +
    "/artifacts/u200_u250/resnet_v1_50_tf/resnet_v1_50_tf.xmodel";

  auto path = default_path;
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

void XModel::doAcquire(ParameterMap* parameters) {
  constexpr auto kThreads = 3;

  auto threads = kThreads;
  if (parameters->has("threads")) {
    threads = parameters->get<int32_t>("threads");
  }
  this->createThreadPool(threads);

  runner_ = vart::Runner::create_runner(this->subgraph_, "run");
  auto input_tensors = runner_->get_input_tensors();
  // assuming only one tensor as in doInit()
  auto input_shape = input_tensors[0]->get_shape();
  auto input_type = mapXirToType(input_tensors[0]->get_data_type());
  this->batch_size_ = input_shape[0];
  this->metadata_.addInputTensor("input", input_shape, input_type);

  auto output_tensors = runner_->get_output_tensors();
  for (const auto* tensor : output_tensors) {
    auto output_shape = tensor->get_shape();
    output_type_.emplace_back(mapXirToType(tensor->get_data_type()));
    // +1 to skip the batch size
    output_size_.emplace_back(
      util::containerProduct(output_shape.begin() + 1, output_shape.end()));
    // TODO(varunsh): what should we return here?
    this->metadata_.addOutputTensor("output", output_shape,
                                    output_type_.back());
  }
}

BatchPtr XModel::doRun(Batch* batch, const MemoryPool* pool) {
#ifdef AMDINFER_ENABLE_TRACING
  for (unsigned int j = 0; j < batch->size(); j++) {
    const auto& trace = batch->getTrace(j);
    trace->startSpan("xmodel");
  }
#endif

  const auto& input_buffers = batch->getInputBuffers();
  // const auto& output_buffers = batch->getOutputBuffers();

#ifdef AMDINFER_ENABLE_LOGGING
  for (const auto& buffer : input_buffers) {
    logTraceBuffer(getLogger(), buffer->data(0));
  }
#endif  // AMDINFER_ENABLE_LOGGING

  std::vector<vart::TensorBuffer*> inputs_ptr;
  for (const auto& buffer : input_buffers) {
    auto* vart = dynamic_cast<VartTensorBuffer*>(buffer.get());
    inputs_ptr.emplace_back(vart->getTensorBuffer());
  }

  BufferPtrs output_buffers;
  BufferPtrs new_input_buffers;
  auto output_tensors = this->getRunner()->get_output_tensors();
  for (const auto* tensor : output_tensors) {
    auto xir_shape = tensor->get_shape();
    std::vector<size_t> shape{xir_shape.begin(), xir_shape.end()};
    auto xir_type = tensor->get_data_type();
    auto type = mapXirToType(xir_type);
    InferenceRequestInput input(nullptr, shape, type, tensor->get_name());
    // the shape includes the batch size so use external batch size 1
    output_buffers.push_back(
      pool->get({MemoryAllocators::VartTensor}, input, 1));
    new_input_buffers.push_back(pool->get(next_allocators_, input, 1));
  }

  std::vector<vart::TensorBuffer*> outputs_ptr;
  for (const auto& buffer : output_buffers) {
    auto* vart = dynamic_cast<VartTensorBuffer*>(buffer.get());
    outputs_ptr.emplace_back(vart->getTensorBuffer());
  }

  // auto outputs_ptr = this->getRunner()->get_outputs();

  for (auto* input : inputs_ptr) {
    const auto* tensor = input->get_tensor();
    auto num = tensor->get_element_num();
    auto batches = (tensor->get_shape())[0];
    input->sync_for_write(0, num / batches);
  }

  std::queue<std::pair<uint32_t, int>> futures;
  futures.push(getRunner()->execute_async(inputs_ptr, outputs_ptr));

  while (!futures.empty()) {
    auto job_id = futures.front();
    futures.pop();
    getRunner()->wait(job_id.first, -1);
  }

  for (auto* output : outputs_ptr) {
    const auto* tensor = output->get_tensor();
    output->sync_for_read(0,
                          tensor->get_element_num() / (tensor->get_shape())[0]);
  }

  const auto num_batches = batch->size();
  auto new_batch = std::make_unique<Batch>();
  for (unsigned int k = 0; k < num_batches; k++) {
    const auto& req = batch->getRequest(k);

    auto new_request = std::make_shared<InferenceRequest>();

    const auto num_outputs = outputs_ptr.size();
    for (unsigned int i = 0; i < num_outputs; i++) {
      auto output_shape = output_tensors[i]->get_shape();
      std::vector<uint64_t> new_shape;
      new_shape.reserve(output_shape.size() - 1);
      for (auto j = 1U; j < output_shape.size(); j++) {
        new_shape.push_back(output_shape[j]);
      }
      auto* output_index =
        reinterpret_cast<void*>(outputs_ptr.at(i)->data().first);

      auto* data_ptr = new_input_buffers.at(i)->data(k * output_size_[i] *
                                                     (output_type_[i]).size());
      new_request->addInputTensor(
        InferenceRequestInput{data_ptr, new_shape, output_type_[i], ""});
      util::copy(
        reinterpret_cast<int8_t*>(output_index) + (i * output_size_[i]),
        static_cast<std::byte*>(data_ptr),
        output_size_[i] * output_type_[i].size());
    }

    new_request->setCallback(req->getCallback());
    new_request->setID(req->getID());
    const auto outputs = req->getOutputs();
    for (const auto& output : outputs) {
      new_request->addOutputTensor(output);
    }

    new_batch->addRequest(new_request);

    new_batch->setModel(k, "xmodel");

    auto& trace = batch->getTrace(static_cast<int>(k));
#ifdef AMDINFER_ENABLE_TRACING
    trace->endSpan();
    new_batch->addTrace(std::move(trace));
#endif

#ifdef AMDINFER_ENABLE_METRICS
    new_batch->addTime(batch->getTime(k));
#endif
  }
  new_batch->setBuffers(std::move(new_input_buffers), {});

  return new_batch;
}

void XModel::doRelease() { this->destroyThreadPool(); }
void XModel::doDestroy() {}

}  // namespace amdinfer::workers

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
amdinfer::workers::Worker* getWorker() {
  return new amdinfer::workers::XModel("XModel", "FPGA", true);
}
}  // extern C
