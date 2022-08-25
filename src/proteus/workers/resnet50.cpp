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
 * @brief Implements the resnet50 worker
 */

#include <aks/AksSysManagerExt.h>  // for SysManagerExt
#include <aks/AksTensorBuffer.h>   // for AksTensorBuffer

#include <algorithm>               // for max
#include <chrono>                  // for microseconds, duration_...
#include <cstddef>                 // for size_t, byte
#include <cstdint>                 // for uint64_t, uint8_t, int32_t
#include <cstring>                 // for memcpy
#include <functional>              // for multiplies
#include <future>                  // for future
#include <memory>                  // for unique_ptr, allocator
#include <numeric>                 // for accumulate
#include <opencv2/core.hpp>        // for Mat, MatSize, Size, Mat...
#include <opencv2/imgcodecs.hpp>   // for imdecode, IMREAD_UNCHANGED
#include <string>                  // for string, basic_string
#include <thread>                  // for thread
#include <utility>                 // for move, pair
#include <vart/tensor_buffer.hpp>  // for TensorBuffer
#include <vector>                  // for vector
#include <xir/tensor/tensor.hpp>   // for Tensor
#include <xir/util/data_type.hpp>  // for create_data_type

#include "proteus/batching/batcher.hpp"       // for BatchPtr, Batch, BatchP...
#include "proteus/buffers/vector_buffer.hpp"  // for VectorBuffer
#include "proteus/build_options.hpp"          // for PROTEUS_ENABLE_TRACING
#include "proteus/core/data_types.hpp"        // for DataType, DataType::UINT32
#include "proteus/core/predict_api.hpp"       // for InferenceResponse, Infe...
#include "proteus/helpers/base64.hpp"         // for base64_decode
#include "proteus/helpers/declarations.hpp"   // for BufferPtrs, InferenceRe...
#include "proteus/helpers/parse_env.hpp"      // for autoExpandEnvironmentVa...
#include "proteus/helpers/thread.hpp"         // for setThreadName
#include "proteus/observation/logging.hpp"    // for Logger
#include "proteus/observation/metrics.hpp"    // for Metrics, MetricSummaryIDs
#include "proteus/observation/tracing.hpp"    // for Trace
#include "proteus/workers/worker.hpp"         // for Worker, kNumBufferAuto

namespace AKS {
class AIGraph;
}  // namespace AKS

uint64_t reduce_mult(std::vector<uint64_t>& v) {
  return std::accumulate(v.begin(), v.end(), 1, std::multiplies<>());
}

namespace proteus {

namespace workers {

/**
 * @brief The Resnet50 worker accepts a 224x224 image and returns an array of
 * classification IDs
 *
 */
class ResNet50 : public Worker {
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

  AKS::SysManagerExt* sysMan_ = nullptr;
  std::string graphName_;
  AKS::AIGraph* graph_ = nullptr;
};

std::thread ResNet50::spawn(BatchPtrQueue* input_queue) {
  return std::thread(&ResNet50::run, this, input_queue);
}

void ResNet50::doInit(RequestParameters* parameters) {
  std::string kGraphName = "resnet50";
  constexpr auto kBatchSize = 4;

  this->sysMan_ = AKS::SysManagerExt::getGlobal();

  auto batch_size = kBatchSize;
  if (parameters->has("batch_size")) {
    batch_size = parameters->get<int32_t>("batch_size");
  }
  this->batch_size_ = batch_size;

  auto graph_name = kGraphName;
  if (parameters->has("aks_graph_name")) {
    graph_name = parameters->get<std::string>("aks_graph_name");
  }
  this->graphName_ = graph_name;
}

constexpr auto kImageWidth = 1920;
constexpr auto kImageHeight = 1080;
constexpr auto kImageChannels = 3;
constexpr auto kImageSize = kImageWidth * kImageHeight * kImageChannels;

size_t ResNet50::doAllocate(size_t num) {
  constexpr auto kBufferNum = 10U;
  size_t buffer_num =
    static_cast<int>(num) == kNumBufferAuto ? kBufferNum : num;
  VectorBuffer::allocate(this->input_buffers_, buffer_num,
                         kImageSize * this->batch_size_, DataType::UINT8);
  VectorBuffer::allocate(this->output_buffers_, kBufferNum,
                         1 * this->batch_size_, DataType::UINT32);
  return buffer_num;
}

void ResNet50::doAcquire(RequestParameters* parameters) {
  auto kPath = std::string(
    "${AKS_ROOT}/graph_zoo/graph_tf_resnet_v1_50_u200_u250_proteus.json");

  auto path = kPath;
  if (parameters->has("aks_graph")) {
    path = parameters->get<std::string>("aks_graph");
  }
  autoExpandEnvironmentVariables(path);
  this->sysMan_->loadGraphs(path);

  this->graph_ = this->sysMan_->getGraph(this->graphName_);

  this->metadata_.addInputTensor(
    "input", DataType::INT8,
    {this->batch_size_, kImageHeight, kImageWidth, kImageChannels});
  // TODO(varunsh): what should we return here?
  this->metadata_.addOutputTensor("output", DataType::UINT32, {0});
  this->metadata_.setName(this->graphName_);
}

void ResNet50::doRun(BatchPtrQueue* input_queue) {
  std::shared_ptr<InferenceRequest> req;
  setThreadName("ResNet50");
#ifdef PROTEUS_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  while (true) {
    BatchPtr batch;
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }
    PROTEUS_LOG_INFO(logger, "Got request in Resnet50");
    std::vector<InferenceResponse> responses;
    responses.reserve(batch->size());

    std::vector<std::unique_ptr<vart::TensorBuffer>> v;
    v.reserve(batch->input_size());

    size_t tensor_count = 0;
    for (unsigned int j = 0; j < batch->size(); j++) {
      const auto& req = batch->getRequest(j);
#ifdef PROTEUS_ENABLE_TRACING
      auto& trace = batch->getTrace(j);
      trace->startSpan("Resnet50");
#endif
      auto& resp = responses.emplace_back();
      resp.setID(req->getID());
      resp.setModel(this->graphName_);
      auto inputs = req->getInputs();
      auto outputs = req->getOutputs();

      uint64_t input_size = 0;
      for (auto& input : inputs) {
        auto* input_buffer = input.getData();

        auto input_shape = input.getShape();

        input_size = reduce_mult(input_shape);
        auto input_dtype = input.getDatatype();

        std::vector<int> tensor_shape = {static_cast<int>(this->batch_size_)};
        if (input_dtype == DataType::UINT8) {
          if (tensor_count == 0) {
            tensor_shape.insert(tensor_shape.end(), input_shape.begin(),
                                input_shape.end());
            v.emplace_back(std::make_unique<AKS::AksTensorBuffer>(
              xir::Tensor::create(this->graphName_, tensor_shape,
                                  xir::create_data_type<unsigned char>())));
          }
          /// Copy input to AKS Buffers: Find a better way to share buffers
          memcpy(reinterpret_cast<uint8_t*>(v[0]->data().first) +
                   (tensor_count * input_size),
                 input_buffer, input_size);
        } else if (input_dtype == DataType::STRING) {
          auto* idata = static_cast<char*>(input_buffer);
          auto decoded_str = base64_decode(idata, input_size);
          std::vector<char> data(decoded_str.begin(), decoded_str.end());
          cv::Mat img = cv::imdecode(data, cv::IMREAD_UNCHANGED);
          if (img.empty()) {
            const char* error = "Decoded image is empty";
            PROTEUS_LOG_ERROR(logger, error);
            req->runCallbackError(error);
            continue;
          }
          // set size to actual size of image instead of size of base64 str
          input_size = img.step[0] * img.rows;

          if (tensor_count == 0) {
            tensor_shape.insert(
              tensor_shape.end(),
              {img.size().height, img.size().width, kImageChannels});
            v.emplace_back(std::make_unique<AKS::AksTensorBuffer>(
              xir::Tensor::create(this->graphName_, tensor_shape,
                                  xir::create_data_type<unsigned char>())));
          }

          // TODO(varunsh): assuming decoded image will be right size
          memcpy(reinterpret_cast<uint8_t*>(v[0]->data().first) +
                   (tensor_count * input_size),
                 img.data, input_size);
        }
        tensor_count = (tensor_count + 1) % this->batch_size_;
      }
    }

    std::future<std::vector<std::unique_ptr<vart::TensorBuffer>>> futureObj =
      this->sysMan_->enqueueJob(this->graph_, "", std::move(v), nullptr);

    auto outDD = futureObj.get();

    int* topKData = reinterpret_cast<int*>(outDD[0]->data().first);
    auto shape = outDD[0]->get_tensor()->get_shape();

    int response_size = 0;
    std::vector<uint64_t> new_shape;
    if (shape.size() > 1) {  // [batch, a, b, c]
      response_size =
        std::accumulate(shape.begin() + 1, shape.end(), 1, std::multiplies<>());
      // We exclude the batch size from the returned shape
      new_shape.reserve(shape.size() - 1);
      for (size_t i = 0; i < shape.size() - 1; i++) {
        new_shape.push_back(shape[i + 1]);
      }
    } else {  // [a]
      new_shape.push_back(shape[0]);
      response_size = shape[0];
    }

    tensor_count = 0;
    for (unsigned int k = 0; k < batch->size(); k++) {
      const auto& req = batch->getRequest(k);
      auto inputs = req->getInputs();
      auto outputs = req->getOutputs();
      auto& resp = responses[k];

      for (unsigned int i = 0; i < inputs.size(); i++) {
        InferenceResponseOutput output;
        std::vector<std::byte> buffer;
        buffer.resize(response_size * sizeof(int));
        memcpy(buffer.data(), topKData + (i * response_size),
               response_size * sizeof(int));
        output.setData(std::move(buffer));

        std::string output_name = outputs[i].getName();
        if (output_name.empty()) {
          output.setName(inputs[i].getName());
        } else {
          output.setName(output_name);
        }

        output.setShape(new_shape);

        output.setDatatype(DataType::UINT32);
        resp.addOutput(output);
      }

#ifdef PROTEUS_ENABLE_METRICS
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - batch->getTime(k));
      Metrics::getInstance().observeSummary(MetricSummaryIDs::kRequestLatency,
                                            duration.count());
#endif
#ifdef PROTEUS_ENABLE_TRACING
      auto context = batch->getTrace(k)->propagate();
      resp.setContext(std::move(context));
#endif
      req->runCallbackOnce(resp);
    }
  }
  PROTEUS_LOG_INFO(logger, "ResNet50 ending");
}

void ResNet50::doRelease() {}
void ResNet50::doDeallocate() {}
void ResNet50::doDestroy() {}

}  // namespace workers

}  // namespace proteus

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
proteus::workers::Worker* getWorker() {
  return new proteus::workers::ResNet50("ResNet50", "AKS");
}
}  // extern C
