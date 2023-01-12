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

#include <dlfcn.h>               // for dlerror, dlopen, dlsym, RTLD...
#include <tensorflow/c/c_api.h>  // for TF_Version
#include <tensorflow/core/framework/graph.pb.h>         // for GraphDef
#include <tensorflow/core/framework/tensor.h>           // for Tensor
#include <tensorflow/core/framework/tensor_shape.h>     // for TensorShape
#include <tensorflow/core/framework/tensor_shape.pb.h>  // for tensorflow
#include <tensorflow/core/framework/tensor_types.h>     // for TTypes<>::Flat
#include <tensorflow/core/framework/types.pb.h>         // for DT_FLOAT
#include <tensorflow/core/platform/env.h>               // for ReadBinaryProto
#include <tensorflow/core/platform/status.h>            // for Status
#include <tensorflow/core/protobuf/config.pb.h>         // for ConfigProto
#include <tensorflow/core/public/session.h>             // for NewSession
#include <tensorflow/core/public/session_options.h>     // for SessionOptions

#include <algorithm>   // for copy, max
#include <cassert>     // for assert
#include <cstddef>     // for size_t, byte
#include <cstdint>     // for int32_t, uint...
#include <cstring>     // for memcpy
#include <functional>  // for multiplies
#include <memory>      // for allocator
#include <numeric>     // for accumulate
#include <ratio>       // for micro, milli
#include <string>      // for string, opera...
#include <thread>      // for thread
#include <utility>     // for pair, move
#include <vector>      // for vector

#include "amdinfer/batching/hard.hpp"          // for Batch, BatchP...
#include "amdinfer/buffers/vector_buffer.hpp"  // for VectorBuffer
#include "amdinfer/build_options.hpp"          // for AMDINFER_ENABL...
#include "amdinfer/core/data_types.hpp"        // for DataType, Dat...
#include "amdinfer/core/exceptions.hpp"        // for external_error
#include "amdinfer/core/predict_api.hpp"       // for InferenceResp...
#include "amdinfer/declarations.hpp"           // for InferenceResp...
#include "amdinfer/observation/logging.hpp"    // for Logger, PROTE...
#include "amdinfer/observation/metrics.hpp"    // for Metrics, Metr...
#include "amdinfer/observation/tracing.hpp"    // for Trace
#include "amdinfer/util/thread.hpp"            // for setThreadName
#include "amdinfer/util/timer.hpp"             // for Timer
#include "amdinfer/workers/worker.hpp"         // for Worker, kNumB...

namespace tf = ::tensorflow;

uint64_t reduceMult(std::vector<uint64_t>& v) {
  return std::accumulate(v.begin(), v.end(), 1, std::multiplies<>());
}

namespace amdinfer::workers {

const int kResNetImageSize = 224;
const int kResNetImageChannels = 3;
const int kResNetOutputClasses = 1000;

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
  tf::Session* session_ = nullptr;
  tf::GraphDef graph_def_;
  tf::Status status_;

  // Image properties
  unsigned int output_classes_ = kResNetOutputClasses;
  unsigned int image_width_ = kResNetImageSize;
  unsigned int image_height_ = kResNetImageSize;
  unsigned int image_channels_ = kResNetImageChannels;
  unsigned int image_size_ = image_width_ * image_height_ * image_channels_;

  // Input / Output nodes
  std::string input_node_{"input"};
  std::string output_node_{"predict"};
  DataType input_dt_ = DataType::Fp32;
};

std::thread TfZendnn::spawn(BatchPtrQueue* input_queue) {
  return std::thread(&TfZendnn::run, this, input_queue);
}

void TfZendnn::doInit(RequestParameters* parameters) {
  const auto default_batch_size = 1;
  const int default_max_buffer_num = 64;

  max_buffer_num_ = default_max_buffer_num;
  if (parameters->has("max_buffer_num")) {
    max_buffer_num_ = parameters->get<int32_t>("max_buffer_num");
  }

  auto batch_size = default_batch_size;
  if (parameters->has("batch_size")) {
    batch_size = parameters->get<int32_t>("batch_size");
  }
  this->batch_size_ = batch_size;

  if (parameters->has("input_size")) {
    image_height_ = parameters->get<int32_t>("input_size");
    image_width_ = parameters->get<int32_t>("input_size");
  }

  if (parameters->has("image_channels")) {
    image_channels_ = parameters->get<int32_t>("image_channels");
  }
  image_size_ = image_height_ * image_width_ * image_channels_;

  if (parameters->has("output_classes")) {
    output_classes_ = parameters->get<int32_t>("output_classes");
  }

  if (parameters->has("input_node")) {
    input_node_ = parameters->get<std::string>("input_node");
  }

  if (parameters->has("output_node")) {
    output_node_ = parameters->get<std::string>("output_node");
  }

  std::string logmsg =
    "TensorFlow C/C++ library version: " + std::string(TF_Version());
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
  AMDINFER_LOG_INFO(logger, logmsg);
#endif
}

size_t TfZendnn::doAllocate(size_t num) {
  const auto default_buffer_num = 10U;
  size_t buffer_num =
    static_cast<int>(num) == kNumBufferAuto ? default_buffer_num : num;

  VectorBuffer::allocate(this->input_buffers_, buffer_num,
                         image_size_ * this->batch_size_, input_dt_);
  VectorBuffer::allocate(this->output_buffers_, buffer_num,
                         output_classes_ * this->batch_size_, DataType::Fp32);
  return buffer_num;
}

void TfZendnn::doAcquire(RequestParameters* parameters) {
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  // TensorFlow session options
  tf::SessionOptions options;
  tf::ConfigProto& config = options.config;
  config.set_use_per_session_threads(false);

  // Parallelism parameters
  const unsigned int default_inter_op = 1;
  const unsigned int default_intra_op = 64;

  auto inter_op = default_inter_op;
  if (parameters->has("inter_op")) {
    inter_op = parameters->get<int>("inter_op");
  }
  auto intra_op = default_intra_op;
  if (parameters->has("intra_op")) {
    inter_op = parameters->get<int>("intra_op");
  }
  config.set_intra_op_parallelism_threads(intra_op);
  config.set_inter_op_parallelism_threads(inter_op);

  // Start a new session
  status_ = tf::NewSession(options, &(this->session_));
  if (!status_.ok()) {
    throw external_error("Could not initialize a tensorflow session");
  }
  AMDINFER_LOG_INFO(logger, "New TF Session Initiated");

  // Load the model
  std::string path;
  if (parameters->has("model")) {
    path = parameters->get<std::string>("model");
  } else {
    throw invalid_argument("Model not provided in load-time parameters");
  }

  status_ = tf::ReadBinaryProto(tf::Env::Default(), path, &graph_def_);
  if (!status_.ok()) {
    throw external_error("Could not load model with tensorflow");
  }
  AMDINFER_LOG_INFO(logger, "Reading Model");

  // Add the graph to the session
  status_ = this->session_->Create(graph_def_);
  if (!status_.ok()) {
    throw external_error("Could not load the model to session");
  }
  AMDINFER_LOG_INFO(logger, "TF Session Created, Ready for prediction");

  // Adding metadata for input and output
  this->metadata_.addInputTensor(
    "input", input_dt_,
    {this->batch_size_, image_height_, image_width_, image_channels_});
  this->metadata_.addOutputTensor("output", DataType::Fp32, {output_classes_});
  this->metadata_.setName("TfZendnn");
}

void TfZendnn::doRun(BatchPtrQueue* input_queue) {
  util::setThreadName("TfZendnn");
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  while (true) {
    std::unique_ptr<Batch> batch;
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }
    AMDINFER_LOG_DEBUG(logger, "Got request in TfZendnn. Size: " +
                                 std::to_string(batch->size()));

    std::vector<InferenceResponse> responses;
    responses.reserve(batch->size());

#ifdef AMDINFER_ENABLE_METRICS
    Metrics::getInstance().incrementCounter(
      MetricCounterIDs::PipelineIngressWorker);
#endif

    util::Timer timer{true};

    // Find total number of images to initialize for TF tensor
    auto tensor_count = static_cast<int>(batch->size());

    // Initialize a TensorFlow tensor with required shape
    tf::Tensor input_tensor(
      tf::DT_FLOAT, tf::TensorShape({tensor_count, image_height_, image_width_,
                                     image_channels_}));

    uint64_t input_size = image_height_ * image_width_ * image_channels_;
    size_t vec_size = 0;

    for (unsigned int j = 0; j < batch->size(); j++) {
      const auto& req = batch->getRequest(j);

#ifdef AMDINFER_ENABLE_TRACING
      const auto& trace = batch->getTrace(j);
      trace->startSpan("tfzendnn");
#endif
      auto& resp = responses.emplace_back();
      resp.setID(req->getID());
      resp.setModel("TFModel");

      auto inputs = req->getInputs();
      auto outputs = req->getOutputs();
      AMDINFER_LOG_DEBUG(logger,
                         "Size of input: " + std::to_string(inputs.size()));

      // Get all the inputs from the requests and copy to the TensorFlow tensor
      for (auto& input : inputs) {
        auto* input_buffer = input.getData();
        const auto* float_buffer = static_cast<float*>(input_buffer);
        std::copy(float_buffer, float_buffer + input_size,
                  input_tensor.flat<float>().data() + vec_size);
        vec_size = vec_size + input_size;
      }
    }

    AMDINFER_LOG_DEBUG(logger, input_tensor.DebugString());

    // Create the inputs and output tensor
    std::vector<std::pair<std::string, tf::Tensor>> input_pair = {
      {input_node_, input_tensor}};
    std::vector<tensorflow::Tensor> output_tensor;

    // Run the session to get the predictions
    timer.add("infer_start");
    status_ =
      this->session_->Run(input_pair, {output_node_}, {}, &output_tensor);
    timer.add("infer_stop");
    auto duration = timer.count<std::milli>("infer_start", "infer_stop");
    AMDINFER_LOG_INFO(logger, "Time taken for " + std::to_string(tensor_count) +
                                " images: " + std::to_string(duration));

    if (!status_.ok()) {
      AMDINFER_LOG_ERROR(logger, status_.ToString());
      for (const auto& req : *batch) {
        req->runCallbackError("Issue with prediction");
      }
    }
    AMDINFER_LOG_DEBUG(logger, output_tensor[0].DebugString());

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

        memcpy(buffer.data(),
               output_tensor[0].flat<float>().data() + (i * response_size),
               response_size * sizeof(float));
        output.setData(std::move(buffer));

        std::string output_name = outputs[i].getName();
        if (output_name.empty()) {
          output.setName(inputs[i].getName());
        } else {
          output.setName(output_name);
        }

        output.setShape(new_shape);
        output.setDatatype(DataType::Fp32);
        resp.addOutput(output);
      }

#ifdef AMDINFER_ENABLE_TRACING
      auto context = batch->getTrace(k)->propagate();
      resp.setContext(std::move(context));
#endif

      timer.stop();
      duration = timer.count<std::milli>();
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
  }
  AMDINFER_LOG_INFO(logger, "TfZendnn ending");
}

void TfZendnn::doRelease() {
  auto retval = this->session_->Close();
  assert(retval.ok());
}
void TfZendnn::doDeallocate() {}
void TfZendnn::doDestroy() {}

void* openLibrary(const char* library, int dlopen_flags) {
  void* handle = dlopen(library, dlopen_flags);
  if (handle == nullptr) {
    const char* error_str = dlerror();
    throw amdinfer::file_not_found_error(error_str);
  }
  return handle;
}

}  // namespace amdinfer::workers

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
amdinfer::workers::Worker* getWorker() {
  using amdinfer::workers::openLibrary;
  // Due to the DEEPBIND change for tensorflow_cc.so below, OMP now gives a
  // segfault unexpectedly if the server is run from Python. Preloading iomp
  // without DEEPBIND addresses this problem.
  openLibrary("libiomp5.so", RTLD_LOCAL | RTLD_LAZY);
  // Upcoming changes in Tensorflow move the Protobuf symbols defined in this
  // library to another library called tensorflow_framework.so. AT runtime,
  // tensorflow_cc then resolves its missing protobuf symbols against the
  // protobuf used in the inference server rather than from
  // tensorflow_framework. Using DEEPBIND addresses this problem so the protobuf
  // symbols get found correctly.
  openLibrary("libtensorflow_cc.so", RTLD_LOCAL | RTLD_LAZY | RTLD_DEEPBIND);

  return new amdinfer::workers::TfZendnn("TfZendnn", "cpu");
}
}  // extern C
