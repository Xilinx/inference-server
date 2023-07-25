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
 * @brief Implements the PyTorch worker
 */

#include <algorithm>   // for copy
#include <cstddef>     // for size_t, byte
#include <cstdint>     // for int32_t, uint64_t
#include <cstring>     // for memcpy
#include <exception>   // for exception
#include <filesystem>  // for path, exists, filesystem
#include <memory>      // for unique_ptr, allocator
#include <ratio>       // for milli, micro
#include <string>      // for string, operator+, to_s...
#include <thread>      // for thread
#include <utility>     // for move
#include <vector>      // for vector

#include "amdinfer/batching/hard.hpp"    // for Batch, BatchPtrQueue
#include "amdinfer/build_options.hpp"    // for AMDINFER_ENABLE_LOGGING
#include "amdinfer/core/data_types.hpp"  // for DataType, DataType::FP32
#include "amdinfer/core/exceptions.hpp"  // for external_error, file_no...
#include "amdinfer/core/inference_request.hpp"   // for InferenceRequest
#include "amdinfer/core/inference_response.hpp"  // for InferenceResponse
#include "amdinfer/core/parameters.hpp"          // for ParameterMap
#include "amdinfer/declarations.hpp"             // for InferenceResponseOutput
#include "amdinfer/observation/logging.hpp"  // for Logger, AMDINFER_LOG_INFO
#include "amdinfer/observation/metrics.hpp"  // for Metrics, MetricCounterIDs
#include "amdinfer/util/containers.hpp"      // for containerProduct
#include "amdinfer/util/memory.hpp"          // for copy
#include "amdinfer/util/thread.hpp"          // for setThreadName
#include "amdinfer/util/timer.hpp"           // for Timer
#include "amdinfer/workers/worker.hpp"       // for Worker, kNumBufferAuto
#include "torch/script.h"                    // for IValue, Tensor, Device

namespace fs = std::filesystem;

namespace amdinfer::workers {

/**
 * @brief The PyTorchWorker worker accepts the name of an PyTorch model file as
 * an argument and compiles and evaluates it.
 *
 */
class PyTorchWorker : public SingleThreadedWorker {
 public:
  using SingleThreadedWorker::SingleThreadedWorker;
  [[nodiscard]] std::vector<MemoryAllocators> getAllocators() const override;

 private:
  void doInit(ParameterMap* parameters) override;
  void doAcquire(ParameterMap* parameters) override;
  BatchPtr doRun(Batch* batch, const MemoryPool* pool) override;
  void doRelease() override;
  void doDestroy() override;

  // workers define what batcher implementation should be used for them.
  // if not explicitly defined here, a default value is used from worker.hpp.
  // using Worker::makeBatcher;
  // std::vector<std::unique_ptr<Batcher>> makeBatcher(
  //   int num, ParameterMap* parameters) override {
  //   return this->makeBatcher<HardBatcher>(num, parameters);
  // };

  // Load the model here
  torch::jit::script::Module model_;

  // Image properties

  std::vector<int32_t> input_shape;
  std::vector<int32_t> batched_input_shape;
  uint64_t input_dim = 1;
  std::vector<int32_t> output_shape;
  std::vector<int32_t> batched_output_shape;
  uint64_t output_dim = 1;

  DataType input_dt_ = DataType::FP32;
};

std::vector<MemoryAllocators> PyTorchWorker::getAllocators() const {
  return {MemoryAllocators::Cpu};
}

std::vector<int32_t> parseShape(std::string shapeStr, uint64_t dim,
                                std::string delimiter) {
  size_t pos = 0;
  std::string token;
  std::vector<int32_t> shape;
  while ((pos = shapeStr.find(delimiter)) != std::string::npos) {
    token = shapeStr.substr(0, pos);
    shape.push_back(std::stoi(token));
    shapeStr.erase(0, pos + delimiter.length());
  }
  shape.push_back(std::stoi(shapeStr));
  if (shape.size() != dim) {
    throw invalid_argument("Shapes size does not equal the dim provided.");
  }
  return shape;
}

void PyTorchWorker::doInit(ParameterMap* parameters) {
  constexpr auto kBatchSize = 1;

  auto batch_size = kBatchSize;
  if (parameters->has("batch_size")) {
    batch_size = parameters->get<int32_t>("batch_size");
    this->batched_input_shape.push_back(batch_size);
    this->batched_output_shape.push_back(batch_size);
  }
  this->batch_size_ = batch_size;

  if (parameters->has("input_dim")) {
    this->input_dim = parameters->get<int32_t>("input_dim");
  } else {
    throw invalid_argument(
      "Num of dimensions of input tensors not provided in load-time "
      "parameters");
  }

  if (parameters->has("input_shape")) {
    std::string shape_str = parameters->get<std::string>("input_shape");
    this->input_shape = parseShape(shape_str, this->input_dim, ",");
    this->batched_input_shape.insert(this->batched_input_shape.end(),
                                     this->input_shape.begin(),
                                     this->input_shape.end());
  } else {
    throw invalid_argument(
      "Shapes of input tensors not provided in load-time parameters");
  }

  if (parameters->has("input_dt")) {
    auto dt = parameters->get<std::string>("input_dt");
    if (dt == "FP32") {
      this->input_dt_ = DataType::FP32;
    } else {
      throw invalid_argument(
        "Unsupported data type provided in load-time parameters");
    }
  } else {
    throw invalid_argument(
      "Data type of input tensors not provided in load-time parameters");
  }

  if (parameters->has("output_dim")) {
    this->output_dim = parameters->get<int32_t>("output_dim");
  } else {
    throw invalid_argument(
      "Num of dimensions of input tensors not provided in load-time "
      "parameters");
  }

  if (parameters->has("output_shape")) {
    std::string shape_str = parameters->get<std::string>("output_shape");
    this->output_shape = parseShape(shape_str, this->output_dim, ",");
    this->batched_output_shape.insert(this->batched_output_shape.end(),
                                      this->output_shape.begin(),
                                      this->output_shape.end());
  } else {
    throw invalid_argument(
      "Shapes of input tensors not provided in load-time parameters");
  }
}

void PyTorchWorker::doAcquire(ParameterMap* parameters) {
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  if (!parameters->has("model")) {
    throw invalid_argument("Model not provided in load-time parameters");
  }
  fs::path path = parameters->get<std::string>("model");
  if (!path.has_extension()) {
    path.replace_extension(".pt");
  }

  if (!fs::exists(path)) {
    throw file_not_found_error("Model " + path.string() + " does not exist");
  }

  // Load the model
  torch::jit::Module torch_module;
  try {
    torch_module = torch::jit::load(path, torch::kCPU);
  } catch (const c10::Error& e) {
    AMDINFER_LOG_ERROR(logger, e.what());
    throw file_read_error("Could not load model with torch");
  }

  AMDINFER_LOG_INFO(logger, "Model loaded");

  // Some online optimizations for the model
  torch_module.eval();
  try {
    torch_module = torch::jit::optimize_for_inference(torch_module);
  } catch (const std::exception& e) {
    AMDINFER_LOG_ERROR(logger, e.what());
    throw external_error("Unable to perform optimizations");
  }
  AMDINFER_LOG_INFO(logger, "Model Optimized, Ready for prediction");

  this->model_ = torch_module;

  // Adding metadata for input and output
  this->metadata_.addInputTensor("input", this->batched_input_shape, input_dt_);
  this->metadata_.addOutputTensor("output", this->batched_output_shape,
                                  DataType::FP32);
  this->metadata_.setName("PyTorchWorker");
}

BatchPtr PyTorchWorker::doRun(Batch* batch, const MemoryPool* pool) {
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  // This ensures no gradient is calculated and provide performance boost
  torch::NoGradGuard no_grad;
  c10::InferenceMode guard;

  size_t input_size = 0;
  std::vector<torch::jit::IValue> input_vec;
  auto tensors = static_cast<int64_t>(batch->size());

  // Initialize a PT tensor with required shape
  std::vector<int64_t> new_input_shape = {tensors};
  std::copy(this->input_shape.begin(), this->input_shape.end(),
            std::back_inserter(new_input_shape));
  torch::Tensor input_tensor = torch::empty(new_input_shape, torch::kF32);

#ifdef AMDINFER_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(
    MetricCounterIDs::PipelineIngressWorker);
#endif
  size_t vec_size = 0;
  util::Timer timer{true};
  for (unsigned int j = 0; j < batch->size(); j++) {
    const auto& req = batch->getRequest(j);

    auto inputs = req->getInputs();
    auto outputs = req->getOutputs();
    AMDINFER_LOG_DEBUG(logger,
                       "Size of input: " + std::to_string(inputs.size()));

    // Get all the inputs from the requests and copy to the PT tensor
    for (const auto& input : inputs) {
      auto* input_buffer = input.getData();
      const auto& input_shape = input.getShape();
      input_size = util::containerProduct(input_shape);

      auto* float_buffer = static_cast<float*>(input_buffer);
      std::copy(float_buffer, float_buffer + input_size,
                input_tensor.data_ptr<float>() + vec_size);
      vec_size = vec_size + input_size;
    }
  }

  // Create the inputs and output tensor
  input_vec.emplace_back(input_tensor);
  c10::IValue prediction;

  // Run through the model to get the predictions
  timer.add("infer_start");
  try {
    prediction = this->model_.forward(input_vec);
  } catch (const c10::Error& e) {
    AMDINFER_LOG_ERROR(logger, "Model not suported/Issue with the model");
    for (const auto& req : *batch) {
      req->runCallbackError("Something went wrong");
    }
  }
  timer.add("infer_stop");
  {
    [[maybe_unused]] auto duration =
      timer.count<std::milli>("infer_start", "infer_stop");
    AMDINFER_LOG_INFO(logger, "Time (ms) taken for " +
                                std::to_string(batch->size()) +
                                " images: " + std::to_string(duration));
  }
  at::Tensor output_tensor;
  if (!prediction.isTuple()) {
    output_tensor = prediction.toTensor();
  } else {
    // For some models like InceptionV3 and GoogleNet which returns Tuple
    output_tensor = prediction.toTuple()->elements()[0].toTensor();
  }

  // Copy the output from the model to the response object
  int64_t response_size = 1;
  for (auto s : this->output_shape) {
    int64_t size = static_cast<int64_t>(s);
    response_size *= size;
  }
  std::vector<int64_t> new_shape = {response_size};

  auto new_batch = batch->propagate();
  std::vector<BufferPtr> input_buffers;
  input_buffers.push_back(pool->get(next_allocators_,
                                    Tensor{"name", new_shape, DataType::Fp32},
                                    batch->size()));

  for (unsigned int k = 0; k < batch->size(); k++) {
    const auto& req = batch->getRequest(k);

    auto new_request = req->propagate();
    auto* data_ptr =
      input_buffers.at(0)->data(k * response_size * DataType("Fp32").size());
    new_request->addInputTensor(
      InferenceRequestInput{data_ptr, new_shape, DataType::Fp32, ""});
    util::copy(output_tensor[0].data_ptr<float>() + (k * response_size),
               static_cast<std::byte*>(data_ptr),
               response_size * sizeof(float));

    new_batch->addRequest(new_request);

    new_batch->setModel(k, "PTModel");
  }

  new_batch->setBuffers(std::move(input_buffers), {});

  return new_batch;
}

void PyTorchWorker::doRelease() {}
void PyTorchWorker::doDestroy() {}

}  // namespace amdinfer::workers

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
amdinfer::workers::Worker* getWorker() {
  return new amdinfer::workers::PyTorchWorker("PyTorchWorker", "CPU", true);
}
}  // extern C
