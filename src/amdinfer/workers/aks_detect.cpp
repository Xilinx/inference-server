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

#include "amdinfer/batching/batcher.hpp"  // for BatchPtr, Batch, BatchP...
#include "amdinfer/build_options.hpp"     // for AMDINFER_ENABLE_TRACING
#include "amdinfer/core/data_types.hpp"   // for DataType, DataType::Uint32
#include "amdinfer/core/inference_request.hpp"   // for InferenceRequest
#include "amdinfer/core/inference_response.hpp"  // for InferenceResponse
#include "amdinfer/core/parameters.hpp"          // for ParameterMap
#include "amdinfer/declarations.hpp"         // for BufferPtrs, InferenceRe...
#include "amdinfer/observation/logging.hpp"  // for Logger
#include "amdinfer/observation/metrics.hpp"  // for Metrics, MetricSummaryIDs
#include "amdinfer/util/base64.hpp"          // for base64_decode
#include "amdinfer/util/containers.hpp"      // for containerProduct
#include "amdinfer/util/memory.hpp"          // for copy
#include "amdinfer/util/parse_env.hpp"       // for autoExpandEnvironmentVa...
#include "amdinfer/util/thread.hpp"          // for setThreadName
#include "amdinfer/util/timer.hpp"           // for Timer
#include "amdinfer/workers/worker.hpp"       // for Worker, kNumBufferAuto

namespace AKS {  // NOLINT(readability-identifier-naming)
class AIGraph;
}  // namespace AKS

namespace amdinfer::workers {

/**
 * @brief The Resnet50 worker accepts a 224x224 image and returns an array of
 * classification IDs
 *
 */
class AksDetect : public SingleThreadedWorker {
 public:
  using SingleThreadedWorker::SingleThreadedWorker;
  [[nodiscard]] std::vector<MemoryAllocators> getAllocators() const override;

 private:
  void doInit(ParameterMap* parameters) override;
  void doAcquire(ParameterMap* parameters) override;
  BatchPtr doRun(Batch* batch, const MemoryPool* pool) override;
  void doRelease() override;
  void doDestroy() override;

  AKS::SysManagerExt* sys_manager_ = nullptr;
  std::string graph_name_;
  AKS::AIGraph* graph_ = nullptr;
};

std::vector<MemoryAllocators> AksDetect::getAllocators() const {
  return {MemoryAllocators::Cpu};
}

void AksDetect::doInit(ParameterMap* parameters) {
  this->sys_manager_ = AKS::SysManagerExt::getGlobal();

  // choose 4 as the default because some DPUs have it fixed to 4
  const auto default_batch_size = 4;
  auto batch_size = default_batch_size;
  if (parameters->has("batch_size")) {
    batch_size = parameters->get<int32_t>("batch_size");
  }
  this->batch_size_ = batch_size;

  std::string graph_name;
  if (parameters->has("aks_graph_name")) {
    graph_name = parameters->get<std::string>("aks_graph_name");
  } else {
    throw invalid_argument(
      "aks_graph_name must be specified in the parameters");
  }
  this->graph_name_ = graph_name;
}

constexpr auto kImageWidth = 1920;
constexpr auto kImageHeight = 1080;
constexpr auto kImageChannels = 3;

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
    "input", {this->batch_size_, kImageHeight, kImageWidth, kImageChannels},
    DataType::Int8);
  this->metadata_.addOutputTensor("output", {0, 6}, DataType::FP32);
  this->metadata_.setName(this->graph_name_);
}

BatchPtr AksDetect::doRun(Batch* batch, const MemoryPool* pool) {
#ifdef AMDINFER_ENABLE_LOGGING
  const auto logger = this->getLogger();
#endif
  const auto batch_size = batch->size();

  std::vector<std::unique_ptr<vart::TensorBuffer>> v;
  v.reserve(batch_size);

  size_t tensor_count = 0;
  for (unsigned int j = 0; j < batch_size; j++) {
    const auto& req = batch->getRequest(static_cast<int>(j));

    auto inputs = req->getInputs();

    uint64_t input_size = 0;
    for (auto& input : inputs) {
      auto* input_buffer = input.getData();

      auto input_shape = input.getShape();

      input_size = util::containerProduct(input_shape);

      std::vector<int> tensor_shape = {static_cast<int>(this->batch_size_)};
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
      tensor_count = (tensor_count + 1) % this->batch_size_;
    }
  }

  std::future<std::vector<std::unique_ptr<vart::TensorBuffer>>> future =
    this->sys_manager_->enqueueJob(this->graph_, "", std::move(v), nullptr);

  auto aks_output = future.get();

  assert(aks_output.size() == 1);
  const auto& aks_tensor_buffer = aks_output.at(0);

  auto aks_tensor_shape = aks_tensor_buffer->get_tensor()->get_shape();
  assert(aks_tensor_shape.empty() || aks_tensor_shape.size() == 2);
  // shape[0] is number of boxes, shape[1] is the number of values per box
  const int size = util::containerProduct(aks_tensor_shape);
  const auto* aks_data =
    reinterpret_cast<float*>(aks_tensor_buffer->data().first);

  // get the max number of boxes for any of the images in the batch and use
  // that to size the next batch
  std::vector<size_t> boxes;
  boxes.resize(batch_size);
  for (int i = 0; i < size; i += kAksDetectResponseSize) {
    auto batch_id = static_cast<int>(aks_data[i]);
    boxes.at(batch_id)++;
  }
  const auto max_boxes = *std::max_element(boxes.begin(), boxes.end());

  auto new_batch = batch->propagate();
  std::vector<BufferPtr> input_buffers;
  input_buffers.push_back(
    pool->get(next_allocators_,
              Tensor{"name", {max_boxes, kDetectResponseSize}, DataType::Fp32},
              batch_size));

  for (auto i = 0U; i < batch_size; ++i) {
    const auto& req = batch->getRequest(i);
    auto new_request = req->propagate();
    new_request->addInputTensor(InferenceRequestInput{
      nullptr, {boxes.at(i), kDetectResponseSize}, DataType::Fp32, ""});
    auto* data_ptr = input_buffers.at(0)->data(
      i * max_boxes * kDetectResponseSize * DataType("Fp32").size());
    new_request->setInputTensorData(0, data_ptr);

    for (auto j = 0; j < size; j += kAksDetectResponseSize) {
      auto batch_id = static_cast<size_t>(aks_data[j]);
      if (batch_id == i) {
        data_ptr =
          util::copy(&(aks_data[j + 1]), static_cast<std::byte*>(data_ptr),
                     sizeof(DetectResponse));
      }
    }

    new_batch->addRequest(new_request);

    new_batch->setModel(i, graph_name_);
  }

  new_batch->setBuffers(std::move(input_buffers), {});

  return new_batch;
}

void AksDetect::doRelease() {}
void AksDetect::doDestroy() {}

}  // namespace amdinfer::workers

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
amdinfer::workers::Worker* getWorker() {
  return new amdinfer::workers::AksDetect("AksDetect", "AKS", true);
}
}  // extern C
