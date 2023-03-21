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
 * @brief Implements the PtZendnn worker
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
#include "amdinfer/observation/tracing.hpp"  // for Trace
#include "amdinfer/util/containers.hpp"      // for containerProduct
#include "amdinfer/util/thread.hpp"          // for setThreadName
#include "amdinfer/util/timer.hpp"           // for Timer
#include "amdinfer/workers/worker.hpp"       // for Worker, kNumBufferAuto
#include "torch/script.h"                    // for IValue, Tensor, Device

namespace fs = std::filesystem;

namespace amdinfer::workers {

const int kResNetImageSize = 224;
const int kResNetImageChannels = 3;
const int kResNetOutputClasses = 1000;

/**
 * @brief The PtZendnn worker is a simple worker that accepts a single uint32_t
 * argument and adds 1 to it and returns. It accepts multiple input tensors and
 * returns the corresponding number of output tensors.
 *
 */
class PtZendnn : public Worker {
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
  unsigned int image_width_ = kResNetImageSize;
  unsigned int image_height_ = kResNetImageSize;
  unsigned int image_channels_ = kResNetImageChannels;
  unsigned int image_size_ = image_width_ * image_height_ * image_channels_;
  unsigned int output_classes_ = kResNetOutputClasses;

  DataType input_dt_ = DataType::FP32;
};

std::thread PtZendnn::spawn(BatchPtrQueue* input_queue) {
  return std::thread(&PtZendnn::run, this, input_queue);
}

std::vector<MemoryAllocators> PtZendnn::getAllocators() const {
  return {MemoryAllocators::Cpu};
}

void PtZendnn::doInit(ParameterMap* parameters) {
  constexpr auto kBatchSize = 1;

  auto batch_size = kBatchSize;
  if (parameters->has("batch_size")) {
    batch_size = parameters->get<int32_t>("batch_size");
  }
  this->batch_size_ = batch_size;

  if (parameters->has("input_size")) {
    image_height_ = parameters->get<int32_t>("input_size");
    image_width_ = parameters->get<int32_t>("input_size");
  }

  if (parameters->has("image_channels")) {
    image_channels_ = parameters->get<int32_t>("image_channels_");
  }

  image_size_ = image_height_ * image_width_ * image_channels_;

  if (parameters->has("output_classes")) {
    output_classes_ = parameters->get<int32_t>("output_classes");
  }
}

void PtZendnn::doAcquire(ParameterMap* parameters) {
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
  this->metadata_.addInputTensor(
    "input", {this->batch_size_, image_height_, image_width_, image_channels_},
    input_dt_);
  this->metadata_.addOutputTensor("output", {0}, DataType::FP32);
  this->metadata_.setName("PtZendnn");
}

void PtZendnn::doRun(BatchPtrQueue* input_queue) {
  std::unique_ptr<Batch> batch;
  util::setThreadName("PtZendnn");
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  while (true) {
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }
    AMDINFER_LOG_DEBUG(logger, "Got request in PtZendnn. Size: " +
                                 std::to_string(batch->size()));

    std::vector<InferenceResponse> responses;
    responses.reserve(batch->size());

    // This ensures no gradient is calculated and provide performance boost
    torch::NoGradGuard no_grad;
    c10::InferenceMode guard;

    size_t input_size = 0;
    std::vector<torch::jit::IValue> input_vec;
    auto tensors = static_cast<int>(batch->size());

    // Initialize a PT tensor with required shape
    torch::Tensor input_tensor = torch::empty(
      {tensors, image_channels_, image_height_, image_width_}, torch::kF32);

#ifdef AMDINFER_ENABLE_METRICS
    Metrics::getInstance().incrementCounter(
      MetricCounterIDs::PipelineIngressWorker);
#endif
    size_t vec_size = 0;
    util::Timer timer{true};
    for (unsigned int j = 0; j < batch->size(); j++) {
      const auto& req = batch->getRequest(j);

#ifdef AMDINFER_ENABLE_TRACING
      const auto& trace = batch->getTrace(j);
      trace->startSpan("ptzendnn");
#endif
      auto& resp = responses.emplace_back();
      resp.setID(req->getID());
      resp.setModel("PTModel");

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
    size_t response_size = output_classes_;
    std::vector<size_t> new_shape = {response_size};
    for (unsigned int k = 0; k < batch->size(); k++) {
      const auto& req = batch->getRequest(k);
      auto inputs = req->getInputs();
      auto outputs = req->getOutputs();
      auto& resp = responses[k];

      for (unsigned int i = 0; i < inputs.size(); i++) {
        InferenceResponseOutput output;
        std::vector<std::byte> buffer;
        buffer.resize(response_size * sizeof(float));

        memcpy(buffer.data(), output_tensor[i].data_ptr<float>(),
               response_size * sizeof(float));
        output.setData(std::move(buffer));

        std::string output_name;
        if (i < outputs.size()) {
          output_name = outputs[i].getName();
        }

        if (output_name.empty()) {
          output.setName(inputs[0].getName());
        } else {
          output.setName(output_name);
        }

        output.setShape(new_shape);
        output.setDatatype(DataType::FP32);
        resp.addOutput(output);
      }

#ifdef AMDINFER_ENABLE_TRACING
      auto context = batch->getTrace(k)->propagate();
      resp.setContext(std::move(context));
#endif
      timer.stop();
      [[maybe_unused]] auto duration = timer.count<std::milli>();
      AMDINFER_LOG_DEBUG(logger,
                         "Total time taken: " + std::to_string(duration));

      req->runCallbackOnce(resp);

#ifdef AMDINFER_ENABLE_METRICS
      timer.add("metrics", batch->getTime(k));
      duration = timer.count<std::micro>("metrics", "stop");
      Metrics::getInstance().observeSummary(MetricSummaryIDs::RequestLatency,
                                            duration);
#endif
    }
    this->returnInputBuffers(std::move(batch));
  }
  AMDINFER_LOG_INFO(logger, "PtZendnn ending");
}

void PtZendnn::doRelease() {}
void PtZendnn::doDestroy() {}

}  // namespace amdinfer::workers

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
amdinfer::workers::Worker* getWorker() {
  return new amdinfer::workers::PtZendnn("PtZendnn", "cpu");
}
}  // extern C
