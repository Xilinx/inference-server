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
 * @brief Implements the AksDetectStream worker
 */

#include <aks/AksSysManagerExt.h>  // for SysManagerExt
#include <aks/AksTensorBuffer.h>   // for AksTensorBuffer

#include <algorithm>               // for copy, max, copy_backward
#include <chrono>                  // for seconds
#include <cstdint>                 // for int32_t, uint8_t
#include <cstring>                 // for size_t, memcpy
#include <ext/alloc_traits.h>      // for __alloc_traits<>::value...
#include <future>                  // for future, future_status
#include <memory>                  // for allocator, unique_ptr
#include <opencv2/core.hpp>        // for Mat, MatSize, Size, Mat...
#include <opencv2/imgcodecs.hpp>   // for imencode
#include <opencv2/videoio.hpp>     // for VideoCapture, VideoCapt...
#include <queue>                   // for queue
#include <string>                  // for string, operator+, to_s...
#include <thread>                  // for thread
#include <utility>                 // for move, pair
#include <vart/tensor_buffer.hpp>  // for TensorBuffer
#include <vector>                  // for vector
#include <xir/tensor/tensor.hpp>   // for Tensor
#include <xir/util/data_type.hpp>  // for create_data_type

#include "amdinfer/batching/batcher.hpp"     // for BatchPtr, Batch, BatchP...
#include "amdinfer/build_options.hpp"        // for AMDINFER_ENABLE_TRACING
#include "amdinfer/core/data_types.hpp"      // for DataType, DataType::String
#include "amdinfer/core/parameters.hpp"      // for ParameterMap
#include "amdinfer/core/predict_api.hpp"     // for InferenceResponse, Infe...
#include "amdinfer/declarations.hpp"         // for BufferPtrs, InferenceRe...
#include "amdinfer/observation/logging.hpp"  // for Logger
#include "amdinfer/observation/tracing.hpp"  // for Trace
#include "amdinfer/util/base64.hpp"          // for base64_encode
#include "amdinfer/util/parse_env.hpp"       // for autoExpandEnvironmentVa...
#include "amdinfer/util/thread.hpp"          // for setThreadName
#include "amdinfer/workers/aks_detect.hpp"   // for DetectResponse
#include "amdinfer/workers/worker.hpp"       // for Worker, kNumBufferAuto

namespace AKS {  // NOLINT(readability-identifier-naming)
class AIGraph;
}  // namespace AKS

namespace amdinfer {

std::string constructMessage(const std::string& key, const std::string& data,
                             const std::string& labels) {
  return R"({"key": ")" + key + R"(", "data": {"img": ")" + data +
         R"(", "labels": )" + labels + "}}";
}

namespace workers {

/**
 * @brief The AksDetectStream worker is a simple worker that accepts an path to
 * a video and sends the inverted frames back to the client over a websocket.
 *
 */
class AksDetectStream : public Worker {
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

  AKS::SysManagerExt* sys_manager_ = nullptr;
  AKS::AIGraph* graph_ = nullptr;
};

std::thread AksDetectStream::spawn(BatchPtrQueue* input_queue) {
  return std::thread(&AksDetectStream::run, this, input_queue);
}

std::vector<MemoryAllocators> AksDetectStream::getAllocators() const {
  return {MemoryAllocators::Cpu};
}

void AksDetectStream::doInit(ParameterMap* parameters) {
  (void)parameters;  // suppress unused variable warning
  constexpr auto kBatchSize = 4;

  /// Get AKS System Manager instance
  this->sys_manager_ = AKS::SysManagerExt::getGlobal();

  this->batch_size_ = kBatchSize;
}

constexpr auto kImageWidth = 1920;
constexpr auto kImageHeight = 1080;
constexpr auto kImageChannels = 3;

void AksDetectStream::doAcquire(ParameterMap* parameters) {
  std::string path{
    "${AKS_ROOT}/graph_zoo/graph_yolov3_u200_u250_amdinfer.json"};
  if (parameters->has("aks_graph")) {
    path = parameters->get<std::string>("aks_graph");
  }
  util::autoExpandEnvironmentVariables(path);
  this->sys_manager_->loadGraphs(path);

  std::string graph_name{"yolov3"};
  if (parameters->has("aks_graph_name")) {
    graph_name = parameters->get<std::string>("aks_graph_name");
  }
  this->graph_ = this->sys_manager_->getGraph(graph_name);

  this->metadata_.addInputTensor(
    "input", DataType::Int8,
    {this->batch_size_, kImageHeight, kImageWidth, kImageChannels});
  // TODO(varunsh): what should we return here?
  this->metadata_.addOutputTensor("output", DataType::Uint32, {0});
  this->metadata_.setName(graph_name);
}

void AksDetectStream::doRun(BatchPtrQueue* input_queue) {
  util::setThreadName("AksDetectStream");
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  while (true) {
    BatchPtr batch;
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }

    AMDINFER_LOG_INFO(logger, "Got request in AksDetectStream");
    for (unsigned int k = 0; k < batch->size(); k++) {
      const auto& req = batch->getRequest(static_cast<int>(k));
#ifdef AMDINFER_ENABLE_TRACING
      const auto& trace = batch->getTrace(static_cast<int>(k));
      trace->startSpan("aks_detect_stream");
#endif
      auto inputs = req->getInputs();
      auto outputs = req->getOutputs();
      auto key = req->getParameters().get<std::string>("key");
      for (auto& input : inputs) {
        auto* input_buffer = input.getData();

        const auto* idata = static_cast<char*>(input_buffer);
        std::string data{idata, input.getSize()};

        cv::VideoCapture cap(data);  // open the video file
        if (!cap.isOpened()) {       // check if we succeeded
          const char* error = "Cannot open video file";
          AMDINFER_LOG_ERROR(logger, error);
          req->runCallbackError(error);
          continue;
        }

        InferenceResponse resp;
        resp.setID(req->getID());
        resp.setModel("aks_detect_stream");

        // contains the number of frames in the video;
        auto count = static_cast<size_t>(
          cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_COUNT));
        if (input.getParameters().has("count")) {
          auto requested_count = input.getParameters().get<int32_t>("count");
          count = std::min(count, static_cast<size_t>(requested_count));
        }
        double fps = cap.get(cv::VideoCaptureProperties::CAP_PROP_FPS);
        auto video_width =
          cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH);
        auto video_height =
          cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT);

        InferenceResponseOutput output;
        output.setName("key");
        output.setDatatype(DataType::String);
        std::string metadata = "[" + std::to_string(video_width) + "," +
                               std::to_string(video_height) + "]";
        auto message = constructMessage(key, std::to_string(fps), metadata);
        output.setData(message.data());
        output.setShape({message.size()});
        resp.addOutput(output);
        req->runCallback(resp);

        // round to nearest multiple of batch size
        auto count_adjusted = count - (count % this->batch_size_);
        std::queue<
          std::future<std::vector<std::unique_ptr<vart::TensorBuffer>>>>
          futures;
        std::queue<std::string> frames;
        for (unsigned int frame_num = 0; frame_num < count_adjusted;
             frame_num += this->batch_size_) {
          std::vector<std::unique_ptr<vart::TensorBuffer>> v;
          v.reserve(1);

#ifdef AMDINFER_ENABLE_TRACING
          trace->startSpan("enqueue_batch");
#endif

          for (size_t i = 0; i < this->batch_size_; i++) {
            cv::Mat frame;
            cap >> frame;  // get the next frame from video
            if (frame.empty()) {
              i--;
              continue;
            }
            if (i == 0) {
              v.emplace_back(
                std::make_unique<AKS::AksTensorBuffer>(xir::Tensor::create(
                  "AksDetect-stream",
                  {static_cast<int>(this->batch_size_), frame.size().height,
                   frame.size().width, kImageChannels},
                  xir::create_data_type<unsigned char>())));
            }
            // cv::resize(frame, frame, cv::Size(kImageWidth, kImageHeight));
            auto input_size = frame.step[0] * frame.rows;
            memcpy(
              reinterpret_cast<uint8_t*>(v[0]->data().first) + (i * input_size),
              frame.data, input_size);

            std::vector<unsigned char> buf;
            cv::imencode(".jpg", frame, buf);
            const auto* enc_msg = reinterpret_cast<const char*>(buf.data());
            std::string encoded = util::base64Encode(enc_msg, buf.size());
            frames.push("data:image/jpg;base64," + encoded);
          }
          AMDINFER_LOG_INFO(logger, "Enqueuing in " + key);
          futures.push(this->sys_manager_->enqueueJob(this->graph_, "",
                                                      std::move(v), nullptr));
#ifdef AMDINFER_ENABLE_TRACING
          trace->endSpan();
#endif
          auto status = futures.front().wait_for(std::chrono::seconds(0));
          if (status == std::future_status::ready) {
            std::vector<std::unique_ptr<vart::TensorBuffer>>
              out_data_descriptor = futures.front().get();
            futures.pop();
            auto* top_k_data =
              reinterpret_cast<float*>(out_data_descriptor[0]->data().first);
            auto shape = out_data_descriptor[0]->get_tensor()->get_shape();
            std::vector<std::string> labels;
            labels.reserve(this->batch_size_);
            for (unsigned int j = 0; j < this->batch_size_; j++) {
              labels.emplace_back("[");
            }
            for (int i = 0; i < shape[0] * shape[1];
                 i += kAkdDetectResponseSize) {
              auto batch_id = static_cast<int>(top_k_data[i]);
              const auto* detect_response =
                reinterpret_cast<DetectResponse*>(&(top_k_data[i + 1]));

              labels[batch_id].append(R"({"fill": false, "box": [)");
              labels[batch_id].append(std::to_string(detect_response->x) + ",");
              labels[batch_id].append(std::to_string(detect_response->y) + ",");
              labels[batch_id].append(std::to_string(detect_response->w) + ",");
              labels[batch_id].append(std::to_string(detect_response->h));
              labels[batch_id].append(R"(], "label": ")");
              labels[batch_id].append(
                std::to_string(detect_response->class_id) + "\"},");
            }
            for (unsigned int j = 0; j < this->batch_size_; j++) {
              if (labels[j].size() > 1) {
                labels[j].pop_back();  // trim trailing comma
              }
              labels[j] += "]";
              InferenceResponse resp;
              resp.setID(req->getID());
              resp.setModel("invert_video");

              InferenceResponseOutput output;
              output.setName("image");
              output.setDatatype(DataType::String);
              auto message = constructMessage(key, frames.front(), labels[j]);
              output.setData(message.data());
              output.setShape({message.size()});
              resp.addOutput(output);
              req->runCallback(resp);
              frames.pop();
            }
          }
        }
        while (!futures.empty()) {
          std::vector<std::unique_ptr<vart::TensorBuffer>> out_data_descriptor =
            futures.front().get();
          AMDINFER_LOG_INFO(logger, "Got future with key " + key);
          futures.pop();
          auto* top_k_data =
            reinterpret_cast<float*>(out_data_descriptor[0]->data().first);
          auto shape = out_data_descriptor[0]->get_tensor()->get_shape();
          std::vector<std::string> labels;
          labels.reserve(this->batch_size_);
          for (unsigned int j = 0; j < this->batch_size_; j++) {
            labels.emplace_back("[");
          }
          for (int i = 0; i < shape[0] * shape[1];
               i += kAkdDetectResponseSize) {
            auto batch_id = static_cast<int>(top_k_data[i]);
            const auto* detect_response =
              reinterpret_cast<DetectResponse*>(&(top_k_data[i + 1]));

            labels[batch_id].append(R"({"fill": false, "box": [)");
            labels[batch_id].append(std::to_string(detect_response->x) + ",");
            labels[batch_id].append(std::to_string(detect_response->y) + ",");
            labels[batch_id].append(std::to_string(detect_response->w) + ",");
            labels[batch_id].append(std::to_string(detect_response->h));
            labels[batch_id].append(R"(], "label": ")");
            labels[batch_id].append(std::to_string(detect_response->class_id) +
                                    "\"},");
          }
          for (unsigned int j = 0; j < this->batch_size_; j++) {
            if (labels[j].size() > 1) {
              labels[j].pop_back();  // trim trailing comma
            }
            labels[j] += "]";
            InferenceResponse resp;
            resp.setID(req->getID());
            resp.setModel("invert_video");

            InferenceResponseOutput output;
            output.setName("image");
            output.setDatatype(DataType::String);
            auto message = constructMessage(key, frames.front(), labels[j]);
            output.setData(message.data());
            output.setShape({message.size()});
            resp.addOutput(output);
            req->runCallback(resp);
            frames.pop();
          }
        }
      }
    }
  }
  AMDINFER_LOG_INFO(logger, "AksDetectStream ending");
}

void AksDetectStream::doRelease() {}
void AksDetectStream::doDestroy() {}

}  // namespace workers

}  // namespace amdinfer

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
amdinfer::workers::Worker* getWorker() {
  return new amdinfer::workers::AksDetectStream("AksDetectStream", "AKS");
}
}  // extern C
