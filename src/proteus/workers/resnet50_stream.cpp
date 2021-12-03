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
 * @brief Implements the ResNet50Stream worker
 */

#include <aks/AksSysManagerExt.h>  // for SysManagerExt
#include <aks/AksTensorBuffer.h>   // for AksTensorBuffer

#include <algorithm>               // for copy, max, copy_backward
#include <chrono>                  // for seconds
#include <cstdint>                 // for int32_t, uint8_t
#include <cstring>                 // for size_t, memcpy
#include <functional>              // for function
#include <future>                  // for future, future_status
#include <memory>                  // for allocator, unique_ptr
#include <opencv2/core.hpp>        // for Mat, Size
#include <opencv2/imgcodecs.hpp>   // for imencode
#include <opencv2/imgproc.hpp>     // for resize
#include <opencv2/videoio.hpp>     // for VideoCapture, CV_CAP_PRO...
#include <queue>                   // for queue
#include <string>                  // for string, operator+, char_...
#include <thread>                  // for thread
#include <utility>                 // for move, pair
#include <vart/tensor_buffer.hpp>  // for TensorBuffer
#include <vector>                  // for vector
#include <xir/tensor/tensor.hpp>   // for Tensor
#include <xir/util/data_type.hpp>  // for create_data_type

#include "proteus/batching/batcher.hpp"       // for Batch, BatchPtrQueue
#include "proteus/buffers/vector_buffer.hpp"  // for VectorBuffer
#include "proteus/core/data_types.hpp"        // for DataType, DataType::STRING
#include "proteus/core/predict_api.hpp"       // for InferenceResponse, Infer...
#include "proteus/helpers/base64.hpp"         // for base64_encode
#include "proteus/helpers/declarations.hpp"   // for BufferPtr, InferenceResp...
#include "proteus/helpers/parse_env.hpp"      // for autoExpandEnvironmentVar...
#include "proteus/helpers/thread.hpp"         // for setThreadName
#include "proteus/observation/logging.hpp"    // for SPDLOG_LOGGER_INFO, SPDL...
#include "proteus/workers/worker.hpp"         // for Worker

namespace AKS {
class AIGraph;
}  // namespace AKS

using vidProps = cv::VideoCaptureProperties;

namespace proteus {

std::string constructMessage(const std::string& key, const std::string& data,
                             const std::string& labels) {
  return R"({"key": ")" + key + R"(", "data": {"img": ")" + data +
         R"(", "labels": )" + labels + "}}";
}

using types::DataType;

namespace workers {

/**
 * @brief The ResNet50Stream worker is a simple worker that accepts an path to a
 * video and sends the inverted frames back to the client over a websocket.
 *
 */
class ResNet50Stream : public Worker {
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

std::thread ResNet50Stream::spawn(BatchPtrQueue* input_queue) {
  return std::thread(&ResNet50Stream::run, this, input_queue);
}

void ResNet50Stream::doInit(RequestParameters* parameters) {
  constexpr auto kBatchSize = 4;
  (void)parameters;  // suppress unused variable warning

  /// Get AKS System Manager instance
  this->sysMan_ = AKS::SysManagerExt::getGlobal();
  this->graphName_ = "resnet50";

  this->batch_size_ = kBatchSize;
}

constexpr auto kImageWidth = 224;
constexpr auto kImageHeight = 224;
constexpr auto kImageChannels = 3;
constexpr auto kImageSize = kImageWidth * kImageHeight * kImageChannels;

constexpr auto kBoxHeight = 10;  // height in pixels

constexpr auto kImageWidthStr = "224";
constexpr auto kBoxHeightStr = "10";

/// number of categories returned for the image
constexpr auto kResnetClassifications = 5;

size_t ResNet50Stream::doAllocate(size_t num) {
  constexpr auto kBufferNum = 10U;
  size_t buffer_num =
    static_cast<int>(num) == kNumBufferAuto ? kBufferNum : num;
  VectorBuffer::allocate(this->input_buffers_, buffer_num,
                         kImageSize * this->batch_size_, DataType::UINT8);
  VectorBuffer::allocate(this->output_buffers_, buffer_num,
                         kImageSize * this->batch_size_, DataType::UINT8);
  return buffer_num;
}

void ResNet50Stream::doAcquire(RequestParameters* parameters) {
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
    "input", types::DataType::INT8,
    {this->batch_size_, kImageHeight, kImageWidth, kImageChannels});
  // TODO(varunsh): what should we return here?
  this->metadata_.addOutputTensor("output", types::DataType::UINT32, {0});
  this->metadata_.setName(this->graphName_);
}

void ResNet50Stream::doRun(BatchPtrQueue* input_queue) {
  std::shared_ptr<InferenceRequest> req;
  std::unique_ptr<Batch> batch;
  setThreadName("ResNet50Stream");

  while (true) {
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }

    SPDLOG_LOGGER_INFO(this->logger_, "Got request in ResNet50Stream");
    for (auto& req : *(batch->requests)) {
      auto inputs = req->getInputs();
      auto outputs = req->getOutputs();
      auto key = req->getParameters()->get<std::string>("key");
      for (auto& input : inputs) {
        auto* input_buffer = input.getData();

        auto* idata = static_cast<char*>(input_buffer);

        cv::VideoCapture cap(idata);  // open the video file
        if (!cap.isOpened()) {        // check if we succeeded
          SPDLOG_LOGGER_WARN(this->logger_, "Cannot open video file");
        }

        InferenceResponse resp;
        resp.setID(req->getID());
        resp.setModel("aks_detect_stream");

        // contains the number of frames in the video;
        auto count =
          static_cast<size_t>(cap.get(vidProps::CAP_PROP_FRAME_COUNT));
        if (input.getParameters()->has("count")) {
          auto requested_count = input.getParameters()->get<int32_t>("count");
          count = std::min(count, static_cast<size_t>(requested_count));
        }
        double fps = cap.get(vidProps::CAP_PROP_FPS);
        auto video_width = cap.get(vidProps::CAP_PROP_FRAME_WIDTH);
        auto video_height = cap.get(vidProps::CAP_PROP_FRAME_HEIGHT);

        InferenceResponseOutput output;
        output.setName("key");
        output.setDatatype(types::DataType::STRING);
        std::string metadata = "[" + std::to_string(video_width) + "," +
                               std::to_string(video_height) + "]";
        auto message = constructMessage(key, std::to_string(fps), metadata);
        output.setData(&message);
        output.setShape({message.size()});
        resp.addOutput(output);
        req->getCallback()(resp);
        // round to nearest multiple of batch size
        auto count_adjusted = count - (count % this->batch_size_);
        std::queue<
          std::future<std::vector<std::unique_ptr<vart::TensorBuffer>>>>
          futures;
        std::queue<std::string> frames;
        for (unsigned int frameNum = 0; frameNum < count_adjusted;
             frameNum += this->batch_size_) {
          std::vector<std::unique_ptr<vart::TensorBuffer>> v;
          v.reserve(1);
          v.emplace_back(std::make_unique<AKS::AksTensorBuffer>(
            xir::Tensor::create("resnet-stream",
                                {static_cast<int>(this->batch_size_),
                                 kImageHeight, kImageWidth, kImageChannels},
                                xir::create_data_type<unsigned char>())));

          for (size_t i = 0; i < this->batch_size_; i++) {
            cv::Mat frame;
            cap >> frame;  // get the next frame from video
            // if (frame.empty()) {
            //   continue;
            // }
            cv::resize(frame, frame, cv::Size(kImageWidth, kImageHeight));

            memcpy(
              reinterpret_cast<uint8_t*>(v[0]->data().first) + (i * kImageSize),
              frame.data, kImageSize);

            std::vector<unsigned char> buf;
            cv::imencode(".jpg", frame, buf);
            const auto* enc_msg = reinterpret_cast<const char*>(buf.data());
            std::string encoded = base64_encode(enc_msg, buf.size());
            frames.push("data:image/jpg;base64," + encoded);
          }
          futures.push(
            this->sysMan_->enqueueJob(this->graph_, "", std::move(v), nullptr));
          auto status = futures.front().wait_for(std::chrono::seconds(0));
          if (status == std::future_status::ready) {
            std::vector<std::unique_ptr<vart::TensorBuffer>> outDD =
              futures.front().get();
            futures.pop();
            int* topKData = reinterpret_cast<int*>(outDD[0]->data().first);
            for (unsigned int i = 0; i < this->batch_size_; i++) {
              std::string labels = "[";
              for (unsigned int j = 0; j < kResnetClassifications; j++) {
                auto y = std::to_string(j * kBoxHeight);
                auto label =
                  std::to_string(topKData[(i * kResnetClassifications) + j]);
                labels += R"({"fill": true, "box": [0,)" + y + "," +
                          kImageWidthStr + "," + kBoxHeightStr +
                          R"(], "label": ")" + label + "\"},";
              }
              labels.pop_back();  // trim trailing comma
              labels += "]";
              InferenceResponse resp;
              resp.setID(req->getID());
              resp.setModel("invert_video");

              InferenceResponseOutput output;
              output.setName("image");
              output.setDatatype(types::DataType::STRING);
              auto message = constructMessage(key, frames.front(), labels);
              output.setData(&message);
              output.setShape({message.size()});
              resp.addOutput(output);
              req->getCallback()(resp);
              frames.pop();
            }
          }
        }
        while (!futures.empty()) {
          std::vector<std::unique_ptr<vart::TensorBuffer>> outDD =
            futures.front().get();
          futures.pop();
          int* topKData = reinterpret_cast<int*>(outDD[0]->data().first);
          for (unsigned int i = 0; i < this->batch_size_; i++) {
            std::string labels = "[";
            for (unsigned int j = 0; j < kResnetClassifications; j++) {
              auto y = std::to_string(j * kBoxHeight);
              auto label =
                std::to_string(topKData[(i * kResnetClassifications) + j]);
              labels += R"({"fill": true, "box": [0,)" + y + "," +
                        kImageWidthStr + "," + kBoxHeightStr +
                        R"(], "label": ")" + label + "\"},";
            }
            labels.pop_back();  // trim trailing comma
            labels += "]";
            InferenceResponse resp;
            resp.setID(req->getID());
            resp.setModel("invert_video");

            InferenceResponseOutput output;
            output.setName("image");
            output.setDatatype(types::DataType::STRING);
            auto message = constructMessage(key, frames.front(), labels);
            output.setData(&message);
            output.setShape({message.size()});
            resp.addOutput(output);
            req->getCallback()(resp);
            frames.pop();
          }
        }
      }
    }
    this->returnBuffers(std::move(batch->input_buffers),
                        std::move(batch->output_buffers));
    SPDLOG_LOGGER_DEBUG(this->logger_, "Returned buffers");
  }
  SPDLOG_LOGGER_INFO(this->logger_, "ResNet50Stream ending");
}

void ResNet50Stream::doRelease() {}
void ResNet50Stream::doDeallocate() {}
void ResNet50Stream::doDestroy() {}

}  // namespace workers

}  // namespace proteus

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
proteus::workers::Worker* getWorker() {
  return new proteus::workers::ResNet50Stream("ResNet50Stream", "AKS");
}
}  // extern C
