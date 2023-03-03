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
 * @brief Implements the AKS detect worker
 */

#include "amdinfer/workers/aks_detect.hpp"

#include <aks/AksSysManagerExt.h>  // for SysManagerExt
#include <aks/AksTensorBuffer.h>   // for AksTensorBuffer

#include <algorithm>               // for max
#include <cstddef>                 // for size_t, byte
#include <cstdint>                 // for uint64_t, uint8_t, int32_t
#include <cstring>                 // for memcpy
#include <exception>               // for exception
#include <future>                  // for future
#include <memory>                  // for unique_ptr, shared_ptr
#include <opencv2/core.hpp>        // for Mat, MatSize, Size, Mat...
#include <opencv2/imgcodecs.hpp>   // for imdecode, IMREAD_UNCHANGED
#include <ratio>                   // for micro
#include <string>                  // for string, basic_string
#include <thread>                  // for thread
#include <utility>                 // for move, pair
#include <vart/tensor_buffer.hpp>  // for TensorBuffer
#include <vector>                  // for vector
#include <xir/tensor/tensor.hpp>   // for Tensor
#include <xir/util/data_type.hpp>  // for create_data_type

#include "amdinfer/batching/batcher.hpp"       // for BatchPtr, Batch, BatchP...
#include "amdinfer/buffers/vector_buffer.hpp"  // for VectorBuffer
#include "amdinfer/build_options.hpp"          // for AMDINFER_ENABLE_TRACING
#include "amdinfer/core/data_types.hpp"        // for DataType, DataType::Uint32
#include "amdinfer/core/parameters.hpp"        // for ParameterMap
#include "amdinfer/core/predict_api.hpp"       // for InferenceResponse, Infe...
#include "amdinfer/declarations.hpp"           // for BufferPtrs, InferenceRe...
#include "amdinfer/observation/logging.hpp"    // for Logger
#include "amdinfer/observation/metrics.hpp"    // for Metrics, MetricSummaryIDs
#include "amdinfer/observation/tracing.hpp"    // for Trace
#include "amdinfer/util/base64.hpp"            // for base64_decode
#include "amdinfer/util/containers.hpp"        // for containerProduct
#include "amdinfer/util/parse_env.hpp"         // for autoExpandEnvironmentVa...
#include "amdinfer/util/thread.hpp"            // for setThreadName
#include "amdinfer/util/timer.hpp"             // for Timer
#include "amdinfer/workers/worker.hpp"         // for Worker, kNumBufferAuto

namespace AKS {  // NOLINT(readability-identifier-naming)
class AIGraph;
}  // namespace AKS

namespace amdinfer::workers {

/**
 * @brief The Resnet50 worker accepts a 224x224 image and returns an array of
 * classification IDs
 *
 */
class AksDetect : public Worker {
 public:
  using Worker::Worker;
  std::thread spawn(BatchPtrQueue* input_queue) override;

 private:
  void doInit(ParameterMap* parameters) override;
  std::vector<MemoryAllocators> doAllocate(size_t num) override;
  void doAcquire(ParameterMap* parameters) override;
  void doRun(BatchPtrQueue* input_queue) override;
  void doRelease() override;
  void doDeallocate() override;
  void doDestroy() override;

  AKS::SysManagerExt* sys_manager_ = nullptr;
  std::string graph_name_;
  AKS::AIGraph* graph_ = nullptr;
};

std::thread AksDetect::spawn(BatchPtrQueue* input_queue) {
  return std::thread(&AksDetect::run, this, input_queue);
}

void AksDetect::doInit(ParameterMap* parameters) {
  this->sys_manager_ = AKS::SysManagerExt::getGlobal();

  // arbitrarily choose 4 as the default
  const auto default_batch_size = 4;
  auto batch_size = default_batch_size;
  if (parameters->has("batch_size")) {
    batch_size = parameters->get<int32_t>("batch_size");
  }
  this->batch_size_ = batch_size;

  std::string graph_name{"yolov3"};
  if (parameters->has("aks_graph_name")) {
    graph_name = parameters->get<std::string>("aks_graph_name");
  }
  this->graph_name_ = graph_name;
}

constexpr auto kImageWidth = 1920;
constexpr auto kImageHeight = 1080;
constexpr auto kImageChannels = 3;
constexpr auto kImageSize = kImageWidth * kImageHeight * kImageChannels;

std::vector<MemoryAllocators> AksDetect::doAllocate(size_t num) {
  (void)num;
  return {MemoryAllocators::Cpu};
}

void AksDetect::doAcquire(ParameterMap* parameters) {
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif
  std::string path{
    "${AKS_ROOT}/graph_zoo/graph_yolov3_u200_u250_amdinfer.json"};
  if (parameters->has("aks_graph")) {
    path = parameters->get<std::string>("aks_graph");
  }
  util::autoExpandEnvironmentVariables(path);

  try {
    this->sys_manager_->loadGraphs(path);
  } catch (const std::exception& e) {
    AMDINFER_LOG_ERROR(logger, e.what());
    throw;
  }

  this->graph_ = this->sys_manager_->getGraph(this->graph_name_);

  this->metadata_.addInputTensor(
    "input", DataType::Int8,
    {this->batch_size_, kImageHeight, kImageWidth, kImageChannels});
  // TODO(varunsh): what should we return here?
  this->metadata_.addOutputTensor("output", DataType::Uint32, {0});
  this->metadata_.setName(this->graph_name_);
}

void AksDetect::doRun(BatchPtrQueue* input_queue) {
  util::setThreadName("AksDetect");
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  while (true) {
    BatchPtr batch;
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }
    AMDINFER_LOG_INFO(logger, "Got request in AksDetect");
    std::vector<InferenceResponse> responses;
    responses.reserve(batch->getInputSize());

    std::vector<std::unique_ptr<vart::TensorBuffer>> v;
    v.reserve(batch->size());

    size_t tensor_count = 0;
    for (unsigned int j = 0; j < batch->size(); j++) {
      const auto& req = batch->getRequest(static_cast<int>(j));
#ifdef AMDINFER_ENABLE_TRACING
      const auto& trace = batch->getTrace(static_cast<int>(j));
      trace->startSpan("AksDetect");
#endif
      auto& resp = responses.emplace_back();
      resp.setID(req->getID());
      resp.setModel(this->graph_name_);

      auto inputs = req->getInputs();
      auto outputs = req->getOutputs();

      uint64_t input_size = 0;
      for (auto& input : inputs) {
        auto* input_buffer = input.getData();

        auto input_shape = input.getShape();

        input_size = util::containerProduct(input_shape);
        auto input_dtype = input.getDatatype();

        std::vector<int> tensor_shape = {static_cast<int>(this->batch_size_)};
        if (input_dtype == DataType::Uint8) {
          if (tensor_count == 0) {
            tensor_shape.insert(tensor_shape.end(), input_shape.begin(),
                                input_shape.end());
            v.emplace_back(std::make_unique<AKS::AksTensorBuffer>(
              xir::Tensor::create(this->graph_name_, tensor_shape,
                                  xir::create_data_type<unsigned char>())));
          }
          /// Copy input to AKS Buffers: Find a better way to share buffers
          memcpy(reinterpret_cast<uint8_t*>(v[0]->data().first) +
                   ((tensor_count % this->batch_size_) * input_size),
                 input_buffer, input_size);
        } else if (input_dtype == DataType::String) {
          auto* idata = static_cast<char*>(input_buffer);
          auto decoded_str = util::base64Decode(idata, input_size);
          std::vector<char> data(decoded_str.begin(), decoded_str.end());
          cv::Mat img = cv::imdecode(data, cv::IMREAD_UNCHANGED);
          if (img.empty()) {
            const char* error = "Decoded image is empty";
            AMDINFER_LOG_ERROR(logger, error);
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
              xir::Tensor::create(this->graph_name_, tensor_shape,
                                  xir::create_data_type<unsigned char>())));
          }

          // TODO(varunsh): assuming decoded image will be right size
          memcpy(reinterpret_cast<uint8_t*>(
                   v[0]->data().first +
                   ((tensor_count % this->batch_size_) * input_size)),
                 img.data, input_size);
        }
        tensor_count = (tensor_count + 1) % this->batch_size_;
      }
    }

    std::future<std::vector<std::unique_ptr<vart::TensorBuffer>>> future =
      this->sys_manager_->enqueueJob(this->graph_, "", std::move(v), nullptr);

    auto out_data_descriptor = future.get();

    auto shape = out_data_descriptor[0]->get_tensor()->get_shape();
    // shape[0] is number of boxes, shape[1] is the number of values per box
    int size = shape.size() > 1 ? shape[0] * shape[1] : 0;

    auto* top_k_data =
      reinterpret_cast<float*>(out_data_descriptor[0]->data().first);
    auto my_data_2 = std::vector<std::vector<std::byte>>();
    my_data_2.resize(this->batch_size_);
    for (int i = 0; i < size; i += kAkdDetectResponseSize) {
      auto batch_id = static_cast<int>(top_k_data[i]);
      auto len = my_data_2[batch_id].size();
      my_data_2[batch_id].resize(len + sizeof(DetectResponse));

      memcpy(my_data_2[batch_id].data() + len, &(top_k_data[i + 1]),
             sizeof(DetectResponse));
    }

    tensor_count = 0;
    for (unsigned int k = 0; k < batch->size(); k++) {
      const auto& req = batch->getRequest(static_cast<int>(k));
      auto inputs = req->getInputs();
      auto outputs = req->getOutputs();
      auto& resp = responses[k];

      // int offset = 0;
      for (unsigned int i = 0; i < inputs.size(); i++) {
        InferenceResponseOutput output;

        output.setDatatype(DataType::Fp32);

        std::string output_name;
        if (i < outputs.size()) {
          output_name = outputs[i].getName();
        }

        if (output_name.empty()) {
          output.setName(inputs[0].getName());
        } else {
          output.setName(output_name);
        }

        output.setShape(
          {kAkdDetectResponseSize - 1,
           my_data_2[tensor_count].size() / sizeof(DetectResponse)});
        output.setData(std::move(my_data_2[tensor_count]));
        resp.addOutput(output);
        tensor_count++;
      }

#ifdef AMDINFER_ENABLE_METRICS
      util::Timer timer{batch->getTime(k)};
      timer.stop();
      auto duration = timer.count<std::micro>();
      Metrics::getInstance().observeSummary(MetricSummaryIDs::RequestLatency,
                                            duration);
#endif

#ifdef AMDINFER_ENABLE_TRACING
      const auto& trace = batch->getTrace(k);
      auto context = trace->propagate();
      resp.setContext(std::move(context));
#endif
      req->runCallbackOnce(resp);
    }
    this->returnInputBuffers(std::move(batch));
  }
  AMDINFER_LOG_INFO(logger, "AksDetect ending");
}

void AksDetect::doRelease() {}
void AksDetect::doDeallocate() {}
void AksDetect::doDestroy() {}

}  // namespace amdinfer::workers

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
amdinfer::workers::Worker* getWorker() {
  return new amdinfer::workers::AksDetect("AksDetect", "AKS");
}
}  // extern C
