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
 * @brief Implements the TfZendnn worker
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
#include "proteus/helpers/string.hpp"         // for endsWith
#include "proteus/helpers/thread.hpp"         // for setThreadName
#include "proteus/observation/logging.hpp"    // for Logger
#include "proteus/observation/metrics.hpp"    // for Metrics
#include "proteus/observation/tracing.hpp"    // for startFollowSpan, SpanPtr
#include "proteus/workers/worker.hpp"         // for Worker
#include "tensorflow/c/c_api.h"               // for TensorFlow version
#include "tensorflow/core/platform/env.h"     // for TensorFlow environment
#include "tensorflow/core/public/session.h"   // for TensorFlow sessions

namespace tf = ::tensorflow;

uint64_t reduce_mult(std::vector<uint64_t>& v) {
  return std::accumulate(v.begin(), v.end(), 1, std::multiplies<>());
}

namespace proteus {

namespace workers {

/**
 * @brief The TfZendnn worker is a simple worker that accepts a single uint32_t
 * argument and adds 1 to it and returns. It accepts multiple input tensors and
 * returns the corresponding number of output tensors.
 *
 */
class TfZendnn : public Worker {
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

  // TF session and graphs
  tf::Session* session_;
  tf::GraphDef graph_def_;
  tf::Status status_;

  // Image properties
  unsigned output_classes_;
  unsigned image_width_, image_height_, image_channels_, image_size_;

  // Input / Output nodes
  std::string input_node_, output_node_;
  DataType input_dt_ = DataType::FP32;
};

std::thread TfZendnn::spawn(BatchPtrQueue* input_queue) {
  return std::thread(&TfZendnn::run, this, input_queue);
}

void TfZendnn::doInit(RequestParameters* parameters) {
  constexpr auto kMaxBufferNum = 64;
  constexpr auto kBatchSize = 1;

  auto max_buffer_num = kMaxBufferNum;
  if (parameters->has("max_buffer_num"))
    max_buffer_num = parameters->get<int32_t>("max_buffer_num");
  this->max_buffer_num_ = max_buffer_num;

  auto batch_size = kBatchSize;
  if (parameters->has("batch_size"))
    batch_size = parameters->get<int32_t>("batch_size");
  this->batch_size_ = batch_size;

  // Image properties
  const unsigned kTotalClasses = 1001;
  const unsigned kImageWidth = 224;
  const unsigned kImageHeight = 224;
  const unsigned kImageChannels = 3;

  if (parameters->has("input_size")) {
    image_height_ = parameters->get<int32_t>("input_size");
    image_width_ = parameters->get<int32_t>("input_size");
  } else {
    image_height_ = kImageWidth;
    image_width_ = kImageHeight;
  }
  if (parameters->has("image_channels"))
    image_channels_ = parameters->get<int32_t>("image_channels");
  else
    image_channels_ = kImageChannels;
  image_size_ = image_height_ * image_width_ * image_channels_;

  if (parameters->has("output_classes"))
    output_classes_ = parameters->get<int32_t>("output_classes");
  else
    output_classes_ = kTotalClasses;

  // Input / Output nodes
  std::string kInputNode = "input";
  std::string kOutputNode = "predict";
  if (parameters->has("input_node"))
    input_node_ = parameters->get<std::string>("input_node");
  else
    input_node_ = kInputNode;
  if (parameters->has("output_node"))
    output_node_ = parameters->get<std::string>("output_node");
  else
    output_node_ = kOutputNode;

  std::string logmsg =
    "TensorFlow C/C++ library version: " + std::string(TF_Version());
#ifdef PROTEUS_ENABLE_LOGGING
  const auto& logger = this->getLogger();
  PROTEUS_LOG_INFO(logger, logmsg);
#endif
}

size_t TfZendnn::doAllocate(size_t num) {
  constexpr auto kBufferNum = 10U;
  size_t buffer_num =
    static_cast<int>(num) == kNumBufferAuto ? kBufferNum : num;

  VectorBuffer::allocate(this->input_buffers_, buffer_num,
                         image_size_ * this->batch_size_, input_dt_);
  VectorBuffer::allocate(this->output_buffers_, buffer_num,
                         output_classes_ * this->batch_size_, DataType::FP32);
  return buffer_num;
}

void TfZendnn::doAcquire(RequestParameters* parameters) {
#ifdef PROTEUS_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  // TensorFlow session options
  tf::SessionOptions options;
  tf::ConfigProto& config = options.config;
  config.set_use_per_session_threads(false);

  // Parallelism parameters
  unsigned inter_op = 1, intra_op = 64;
  if (parameters->has("inter_op")) inter_op = parameters->get<int>("inter_op");
  if (parameters->has("intra_op")) inter_op = parameters->get<int>("intra_op");
  config.set_intra_op_parallelism_threads(intra_op);
  config.set_inter_op_parallelism_threads(inter_op);

  // Start a new session
  status_ = tf::NewSession(options, &(this->session_));
  if (!status_.ok()) {
    throw external_error("Could not initialize a tensorflow session");
  }
  PROTEUS_LOG_INFO(logger, "New TF Session Initiated");

  // Load the model
  std::string path;
  if (parameters->has("model")) {
    path = parameters->get<std::string>("model");
    if (!endsWith(path, ".pb")) {
      path += ".pb";
    }
  } else {
    throw invalid_argument("Model not provided in load-time parameters");
  }

  status_ = tf::ReadBinaryProto(tf::Env::Default(), path, &graph_def_);
  if (!status_.ok()) {
    throw external_error("Could not load model with tensorflow");
  }
  PROTEUS_LOG_INFO(logger, "Reading Model");

  // Add the graph to the session
  status_ = this->session_->Create(graph_def_);
  if (!status_.ok()) {
    throw external_error("Could not load the model to session");
  }
  PROTEUS_LOG_INFO(logger, "TF Session Created, Ready for prediction");

  // Adding metadata for input and output
  this->metadata_.addInputTensor(
    "input", input_dt_,
    {this->batch_size_, image_height_, image_width_, image_channels_});
  this->metadata_.addOutputTensor("output", DataType::FP32, {output_classes_});
  this->metadata_.setName("TfZendnn");
}

void TfZendnn::doRun(BatchPtrQueue* input_queue) {
  std::shared_ptr<InferenceRequest> req;
  std::unique_ptr<Batch> batch;
  setThreadName("TfZendnn");
#ifdef PROTEUS_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  while (true) {
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }
    PROTEUS_LOG_DEBUG(logger, "Got request in TfZendnn. Size: " +
                                std::to_string(batch->requests->size()));

    std::vector<InferenceResponse> responses;
    responses.reserve(batch->requests->size());

#ifdef PROTEUS_ENABLE_METRICS
    Metrics::getInstance().incrementCounter(
      MetricCounterIDs::kPipelineIngressWorker);
#endif

    auto TotalStart = std::chrono::high_resolution_clock::now();

    // Find total number of images to initialize for TF tensor
    unsigned tensor_count = 0;
    for (unsigned int j = 0; j < batch->requests->size(); j++) {
      auto req = batch->requests->at(j);
      tensor_count += req->getInputs().size();
    }

    // Initialize a TensorFlow tensor with required shape
    tf::Tensor input_tensor(
      tf::DT_FLOAT, tf::TensorShape({tensor_count, image_height_, image_width_,
                                     image_channels_}));

    uint64_t input_size = image_height_ * image_width_ * image_channels_;
    size_t vec_size = 0;

    for (unsigned int j = 0; j < batch->requests->size(); j++) {
      auto& req = batch->requests->at(j);

#ifdef PROTEUS_ENABLE_TRACING
      auto& trace = batch->traces.at(j);
      trace->startSpan("tfzendnn");
#endif
      auto& resp = responses.emplace_back();
      resp.setID(req->getID());
      resp.setModel("TFModel");

      auto inputs = req->getInputs();
      auto outputs = req->getOutputs();
      PROTEUS_LOG_DEBUG(logger,
                        "Size of input: " + std::to_string(inputs.size()));

      // Get all the inputs from the requests and copy to the TensorFlow tensor
      for (auto& input : inputs) {
        auto* input_buffer = input.getData();
        const float* floatBuffer = (float*)input_buffer;
        std::copy(floatBuffer, floatBuffer + input_size,
                  input_tensor.flat<float>().data() + vec_size);
        vec_size = vec_size + input_size;
      }
    }

    PROTEUS_LOG_DEBUG(logger, input_tensor.DebugString());

    // Create the inputs and output tensor
    std::vector<std::pair<std::string, tf::Tensor>> input_pair = {
      {input_node_, input_tensor}};
    std::vector<tensorflow::Tensor> output_tensor;

    // Run the session to get the predictions
    auto start = std::chrono::high_resolution_clock::now();
    status_ =
      this->session_->Run(input_pair, {output_node_}, {}, &output_tensor);
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
#ifdef PROTEUS_ENABLE_LOGGING
    float time_tmp = duration.count();
    PROTEUS_LOG_INFO(logger, "Time taken for " + std::to_string(tensor_count) +
                               " images: " + std::to_string(time_tmp));
#endif

    if (!status_.ok()) {
      PROTEUS_LOG_ERROR(logger, status_.ToString());
      req->runCallbackError("Issue with prediction");
    }
    PROTEUS_LOG_DEBUG(logger, output_tensor[0].DebugString());

    // Copy the output from the model to the response object
    size_t response_size = output_classes_;
    std::vector<size_t> new_shape = {response_size};
    for (unsigned int k = 0; k < batch->requests->size(); k++) {
      auto req = (*batch->requests)[k];
      auto inputs = req->getInputs();
      auto outputs = req->getOutputs();
      auto& resp = responses[k];

      for (unsigned int i = 0; i < inputs.size(); i++) {
        InferenceResponseOutput output;
        auto buffer = std::make_shared<std::vector<float>>();
        buffer->resize(response_size);

        memcpy(buffer->data(),
               output_tensor[0].flat<float>().data() + (i * response_size),
               response_size * sizeof(float));
        auto my_data_cast = std::reinterpret_pointer_cast<std::byte>(buffer);
        output.setData(my_data_cast);

        std::string output_name = outputs[i].getName();
        if (output_name.empty()) {
          output.setName(inputs[i].getName());
        } else {
          output.setName(output_name);
        }

        output.setShape(new_shape);
        output.setDatatype(DataType::FP32);
        resp.addOutput(output);
      }

#ifdef PROTEUS_ENABLE_TRACING
      auto context = batch->traces.at(k)->propagate();
      resp.setContext(std::move(context));
#endif

      auto TotalStop = std::chrono::high_resolution_clock::now();
      auto d = std::chrono::duration_cast<std::chrono::milliseconds>(
        TotalStop - TotalStart);
#ifdef PROTEUS_ENABLE_LOGGING
      float tt = d.count();
      PROTEUS_LOG_DEBUG(logger, "Total time taken: " + std::to_string(tt));
#endif

      req->runCallbackOnce(resp);

#ifdef PROTEUS_ENABLE_METRICS
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - batch->start_times[k]);
      Metrics::getInstance().observeSummary(MetricSummaryIDs::kRequestLatency,
                                            duration.count());
#endif
    }
    this->returnBuffers(std::move(batch->input_buffers),
                        std::move(batch->output_buffers));
    PROTEUS_LOG_DEBUG(logger, "Returned buffers");
  }
  PROTEUS_LOG_INFO(logger, "TfZendnn ending");
}

void TfZendnn::doRelease() { this->session_->Close(); }
void TfZendnn::doDeallocate() {}
void TfZendnn::doDestroy() {}

}  // namespace workers

}  // namespace proteus

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
proteus::workers::Worker* getWorker() {
  return new proteus::workers::TfZendnn("TfZendnn", "cpu");
}
}  // extern C
