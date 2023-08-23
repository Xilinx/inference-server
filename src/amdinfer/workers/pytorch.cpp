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
#include <regex>       // for regex
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

  torch::jit::script::Module model_;

  // Image properties

  std::vector<std::vector<int64_t>> input_shapes_;
  std::vector<std::vector<int64_t>> output_shapes_;
  DataType input_datatype_ = DataType::FP32;
  DataType output_datatype_ = DataType::FP32;
  // std::vector<int32_t> input_shape;
  // std::vector<int32_t> batched_input_shape;
  // uint64_t input_dim = 1;
  // std::vector<int32_t> output_shape;
  // std::vector<int32_t> batched_output_shape;
  // uint64_t output_dim = 1;
  std::string device_ = "CPU";
};

std::vector<MemoryAllocators> PyTorchWorker::getAllocators() const {
  return {MemoryAllocators::Cpu};
}

std::vector<std::vector<int64_t>> parseShape(const std::string& shape_str,
                                             size_t batch_size) {
  std::vector<std::vector<int64_t>> shapes;
  std::regex semicolon_regex(";");
  std::regex comma_regex(",");
  std::sregex_token_iterator tensor_iter{shape_str.begin(), shape_str.end(),
                                         semicolon_regex, -1};
  std::sregex_token_iterator tensor_end;
  while (tensor_iter != tensor_end) {
    auto match = *tensor_iter;
    std::sregex_token_iterator dim_iter{match.first, match.second, comma_regex,
                                        -1};
    std::sregex_token_iterator dim_end;
    std::vector<int64_t> tensor;
    tensor.push_back(batch_size);
    while (dim_iter != dim_end) {
      tensor.push_back(std::stoi(*dim_iter));
      dim_iter++;
    }
    shapes.push_back(tensor);
    tensor_iter++;
  }
  return shapes;
}

void PyTorchWorker::doInit(ParameterMap* parameters) {
  if (parameters->has("batch_size")) {
    batch_size_ = parameters->get<int32_t>("batch_size");
  }

  if (parameters->has("device")) {
    device_ = parameters->get<std::string>("device");
  }

  if (parameters->has("input_shapes")) {
    auto shape_str = parameters->get<std::string>("input_shapes");
    input_shapes_ = parseShape(shape_str, batch_size_);
  } else {
    throw invalid_argument(
      "Shapes of input tensors not provided in load-time parameters");
  }

  if (parameters->has("input_datatype")) {
    auto datatype = parameters->get<std::string>("input_datatype");
    input_datatype_ = DataType(datatype.c_str());
  }

  if (parameters->has("output_datatype")) {
    auto datatype = parameters->get<std::string>("output_datatype");
    output_datatype_ = DataType(datatype.c_str());
  }

  if (parameters->has("output_shapes")) {
    std::string shape_str = parameters->get<std::string>("output_shapes");
    output_shapes_ = parseShape(shape_str, batch_size_);
  } else {
    throw invalid_argument(
      "Shapes of output tensors not provided in load-time parameters");
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
    c10::DeviceType deviceType;
    if (device_ == "CPU") {
      deviceType = torch::kCPU;
    } else {
      throw invalid_argument("Invalid target device: " + device_);
    }
    torch_module = torch::jit::load(path, deviceType);
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

  model_ = torch_module;

  // Adding metadata for input and output
  for (const auto& tensor : input_shapes_) {
    metadata_.addInputTensor("input", tensor, input_datatype_);
  }
  for (const auto& tensor : input_shapes_) {
    metadata_.addOutputTensor("output", tensor, output_datatype_);
  }

  metadata_.setName("PyTorchWorker");
}

BatchPtr PyTorchWorker::doRun(Batch* batch, const MemoryPool* pool) {
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  // This ensures no gradient is calculated and provide performance boost
  torch::NoGradGuard no_grad;
  c10::InferenceMode guard;

  std::vector<torch::jit::IValue> input_vec;
  auto tensors = static_cast<int64_t>(batch->size());

  const auto& input_buffers = batch->getInputBuffers();

  int i = 0;
  util::Timer timer{true};
  for (const auto& input_buffer : input_buffers) {
    // Initialize a PT tensor with required shape
    std::vector<int64_t> new_input_shape = {tensors};
    auto input_shape = batch->getRequest(0)->getInputs()[i].getShape();
    // auto input_shape = {3, 224, 224};
    std::copy(input_shape.begin(), input_shape.end(),
              std::back_inserter(new_input_shape));
    torch::Tensor input_tensor = torch::empty(new_input_shape, torch::kF32);

#ifdef AMDINFER_ENABLE_METRICS
    Metrics::getInstance().incrementCounter(
      MetricCounterIDs::PipelineIngressWorker);
#endif
    auto* data = static_cast<float*>(input_buffer->data(0));
    size_t input_size = util::containerProduct(new_input_shape);
    std::copy(data, data + input_size, input_tensor.data_ptr<float>());
    // size_t vec_size = 0;

    // for (unsigned int j = 0; j < batch->size(); j++) {
    //   const auto& req = batch->getRequest(j);

    //   auto inputs = req->getInputs();
    //   auto outputs = req->getOutputs();
    //   AMDINFER_LOG_DEBUG(logger,
    //                     "Size of input: " + std::to_string(inputs.size()));

    //   // Get all the inputs from the requests and copy to the PT tensor
    //   for (const auto& input : inputs) {
    //     auto* input_buffer = input.getData();
    //     input_size = util::containerProduct(input.getShape());

    //     auto* float_buffer = static_cast<float*>(input_buffer);
    //     std::copy(float_buffer, float_buffer + input_size,
    //               input_tensor.data_ptr<float>() + vec_size);
    //     vec_size = vec_size + input_size;
    //   }
    // }

    // Create the inputs and output tensor
    input_vec.emplace_back(input_tensor);
    i++;
  }
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
  std::vector<at::Tensor> output_tensors;
  if (prediction.isTensor()) {
    output_tensors.push_back(prediction.toTensor());
  } else if (prediction.isTuple()) {
    // For some models like InceptionV3 and GoogleNet which returns Tuple
    output_tensors.push_back(prediction.toTuple()->elements()[0].toTensor());
  } else if (prediction.isTensorList()) {
    output_tensors = prediction.toTensorVector();
  } else {
    assert(false);
  }

  // Copy the output from the model to the response object
  auto new_batch = batch->propagate();
  std::vector<BufferPtr> new_input_buffers;

  int k = 0;
  for (const auto& output_tensor : output_tensors) {
    auto response_shape = output_tensor.sizes();
    std::vector<int64_t> new_shape{response_shape.begin() + 1,
                                   response_shape.end()};
    auto response_size = util::containerProduct(new_shape);
    new_input_buffers.push_back(
      pool->get(next_allocators_, Tensor{"name", new_shape, DataType::Fp32},
                batch->size()));

    const auto& req = batch->getRequest(k);

    auto new_request = req->propagate();
    auto* data_ptr = new_input_buffers.at(k)->data(k * response_size *
                                                   DataType("Fp32").size());
    new_request->addInputTensor(
      InferenceRequestInput{data_ptr, new_shape, DataType::Fp32, ""});
    util::copy(output_tensor[k].data_ptr<float>() + (k * response_size),
               static_cast<std::byte*>(data_ptr),
               response_size * sizeof(float));

    new_batch->addRequest(new_request);

    new_batch->setModel(k, "PTModel");
    k++;
  }

  new_batch->setBuffers(std::move(new_input_buffers), {});

  return new_batch;
}

void PyTorchWorker::doRelease() {}
void PyTorchWorker::doDestroy() {}

}  // namespace amdinfer::workers

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
amdinfer::workers::Worker* getWorker() {
  return new amdinfer::workers::PyTorchWorker("PyTorchWorker", "Multiple",
                                              true);
}
}  // extern C
