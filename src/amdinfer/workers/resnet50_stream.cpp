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
 * @brief Implements the ResNet50Stream worker
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
#include <opencv2/core.hpp>        // for Mat, Size
#include <opencv2/imgcodecs.hpp>   // for imencode
#include <opencv2/imgproc.hpp>     // for resize
#include <opencv2/videoio.hpp>     // for VideoCapture, VideoCapt...
#include <queue>                   // for queue
#include <string>                  // for string, operator+, char...
#include <thread>                  // for thread
#include <utility>                 // for move, pair
#include <vart/tensor_buffer.hpp>  // for TensorBuffer
#include <vector>                  // for vector
#include <xir/tensor/tensor.hpp>   // for Tensor
#include <xir/util/data_type.hpp>  // for create_data_type

#include "amdinfer/batching/batcher.hpp"       // for BatchPtr, BatchPtrQueue
#include "amdinfer/buffers/vector_buffer.hpp"  // for VectorBuffer
#include "amdinfer/build_options.hpp"          // for AMDINFER_ENABLE_LOGGING
#include "amdinfer/core/data_types.hpp"        // for DataType, DataType::String
#include "amdinfer/core/predict_api.hpp"       // for InferenceResponse, Infe...
#include "amdinfer/declarations.hpp"           // for BufferPtrs, InferenceRe...
#include "amdinfer/observation/logging.hpp"    // for Logger
#include "amdinfer/util/base64.hpp"            // for base64_encode
#include "amdinfer/util/parse_env.hpp"         // for autoExpandEnvironmentVa...
#include "amdinfer/util/thread.hpp"            // for setThreadName
#include "amdinfer/workers/worker.hpp"         // for Worker, kNumBufferAuto

namespace AKS {  // NOLINT(readability-identifier-naming)
class AIGraph;
}  // namespace AKS

using VidProps = cv::VideoCaptureProperties;

namespace amdinfer {

std::string constructMessage(const std::string& key, const std::string& data,
                             const std::string& labels) {
  return R"({"key": ")" + key + R"(", "data": {"img": ")" + data +
         R"(", "labels": )" + labels + "}}";
}

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

  AKS::SysManagerExt* sys_manager_ = nullptr;
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
  this->sys_manager_ = AKS::SysManagerExt::getGlobal();
  this->graphName_ = "resnet50";

  this->batch_size_ = kBatchSize;
}

constexpr auto kImageWidth = 224;
constexpr auto kImageHeight = 224;
constexpr auto kImageChannels = 3;
constexpr auto kImageSize = kImageWidth * kImageHeight * kImageChannels;

constexpr auto kBoxHeight = 10;  // height in pixels

const std::string kImageWidthStr{"224"};
const std::string kBoxHeightStr{"10"};

/// number of categories returned for the image
constexpr auto kResnetClassifications = 5;

size_t ResNet50Stream::doAllocate(size_t num) {
  constexpr auto kBufferNum = 10U;
  size_t buffer_num =
    static_cast<int>(num) == kNumBufferAuto ? kBufferNum : num;
  VectorBuffer::allocate(this->input_buffers_, buffer_num,
                         kImageSize * this->batch_size_, DataType::Uint8);
  VectorBuffer::allocate(this->output_buffers_, buffer_num,
                         kImageSize * this->batch_size_, DataType::Uint8);
  return buffer_num;
}

void ResNet50Stream::doAcquire(RequestParameters* parameters) {
  std::string path{
    "${AKS_ROOT}/graph_zoo/graph_tf_resnet_v1_50_u200_u250_amdinfer.json"};
  if (parameters->has("aks_graph")) {
    path = parameters->get<std::string>("aks_graph");
  }
  util::autoExpandEnvironmentVariables(path);
  this->sys_manager_->loadGraphs(path);

  this->graph_ = this->sys_manager_->getGraph(this->graphName_);

  this->metadata_.addInputTensor(
    "input", DataType::Int8,
    {this->batch_size_, kImageHeight, kImageWidth, kImageChannels});
  // TODO(varunsh): what should we return here?
  this->metadata_.addOutputTensor("output", DataType::Uint32, {0});
  this->metadata_.setName(this->graphName_);
}

void ResNet50Stream::doRun(BatchPtrQueue* input_queue) {
  std::shared_ptr<InferenceRequest> req;
  util::setThreadName("ResNet50Stream");
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  while (true) {
    BatchPtr batch;
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }

    AMDINFER_LOG_INFO(logger, "Got request in ResNet50Stream");
    for (const auto& req : *batch) {
      auto inputs = req->getInputs();
      auto outputs = req->getOutputs();
      auto key = req->getParameters()->get<std::string>("key");
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
        auto count =
          static_cast<size_t>(cap.get(VidProps::CAP_PROP_FRAME_COUNT));
        if (input.getParameters()->has("count")) {
          auto requested_count = input.getParameters()->get<int32_t>("count");
          count = std::min(count, static_cast<size_t>(requested_count));
        }
        double fps = cap.get(VidProps::CAP_PROP_FPS);
        auto video_width = cap.get(VidProps::CAP_PROP_FRAME_WIDTH);
        auto video_height = cap.get(VidProps::CAP_PROP_FRAME_HEIGHT);

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
        for (unsigned int num_frames = 0; num_frames < count_adjusted;
             num_frames += this->batch_size_) {
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
            std::string encoded = util::base64Encode(enc_msg, buf.size());
            frames.push("data:image/jpg;base64," + encoded);
          }
          futures.push(this->sys_manager_->enqueueJob(this->graph_, "",
                                                      std::move(v), nullptr));
          auto status = futures.front().wait_for(std::chrono::seconds(0));
          if (status == std::future_status::ready) {
            std::vector<std::unique_ptr<vart::TensorBuffer>>
              out_data_descriptor = futures.front().get();
            futures.pop();
            int* top_k_data =
              reinterpret_cast<int*>(out_data_descriptor[0]->data().first);
            for (unsigned int i = 0; i < this->batch_size_; i++) {
              std::string labels = "[";
              for (unsigned int j = 0; j < kResnetClassifications; j++) {
                auto y = std::to_string(j * kBoxHeight);
                auto label =
                  std::to_string(top_k_data[(i * kResnetClassifications) + j]);
                labels.append(R"({"fill": true, "box": [0,)");
                labels.append(y + ",");
                labels.append(kImageWidthStr + ",");
                labels.append(kBoxHeightStr + R"(], "label": ")");
                labels.append(label + "\"},");
              }
              labels.pop_back();  // trim trailing comma
              labels += "]";
              InferenceResponse resp;
              resp.setID(req->getID());
              resp.setModel("invert_video");

              InferenceResponseOutput output;
              output.setName("image");
              output.setDatatype(DataType::String);
              auto message = constructMessage(key, frames.front(), labels);
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
          futures.pop();
          int* top_k_data =
            reinterpret_cast<int*>(out_data_descriptor[0]->data().first);
          for (unsigned int i = 0; i < this->batch_size_; i++) {
            std::string labels = "[";
            for (unsigned int j = 0; j < kResnetClassifications; j++) {
              auto y = std::to_string(j * kBoxHeight);
              auto label =
                std::to_string(top_k_data[(i * kResnetClassifications) + j]);
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
            output.setDatatype(DataType::String);
            auto message = constructMessage(key, frames.front(), labels);
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
  AMDINFER_LOG_INFO(logger, "ResNet50Stream ending");
}

void ResNet50Stream::doRelease() {}
void ResNet50Stream::doDeallocate() {}
void ResNet50Stream::doDestroy() {}

}  // namespace workers

}  // namespace amdinfer

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
amdinfer::workers::Worker* getWorker() {
  return new amdinfer::workers::ResNet50Stream("ResNet50Stream", "AKS");
}
}  // extern C
