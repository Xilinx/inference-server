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

#include "proteus/batching/batcher.hpp"       // for BatchPtr, Batch, BatchP...
#include "proteus/buffers/vector_buffer.hpp"  // for VectorBuffer
#include "proteus/build_options.hpp"          // for PROTEUS_ENABLE_TRACING
#include "proteus/core/data_types.hpp"        // for DataType, DataType::STRING
#include "proteus/core/predict_api.hpp"       // for InferenceResponse, Infe...
#include "proteus/helpers/base64.hpp"         // for base64_encode
#include "proteus/helpers/declarations.hpp"   // for BufferPtrs, InferenceRe...
#include "proteus/helpers/parse_env.hpp"      // for autoExpandEnvironmentVa...
#include "proteus/helpers/thread.hpp"         // for setThreadName
#include "proteus/observation/logging.hpp"    // for SPDLOG_LOGGER_INFO, SPD...
#include "proteus/observation/tracing.hpp"    // for Trace
#include "proteus/workers/worker.hpp"         // for Worker, kNumBufferAuto

namespace AKS {
class AIGraph;
}  // namespace AKS

namespace proteus {

std::string constructMessage(const std::string& key, const std::string& data,
                             const std::string& labels) {
  return R"({"key": ")" + key + R"(", "data": {"img": ")" + data +
         R"(", "labels": )" + labels + "}}";
}

using types::DataType;

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

 private:
  void doInit(RequestParameters* parameters) override;
  size_t doAllocate(size_t num) override;
  void doAcquire(RequestParameters* parameters) override;
  void doRun(BatchPtrQueue* input_queue) override;
  void doRelease() override;
  void doDeallocate() override;
  void doDestroy() override;

  AKS::SysManagerExt* sysMan_ = nullptr;
  AKS::AIGraph* graph_ = nullptr;
};

std::thread AksDetectStream::spawn(BatchPtrQueue* input_queue) {
  return std::thread(&AksDetectStream::run, this, input_queue);
}

void AksDetectStream::doInit(RequestParameters* parameters) {
  (void)parameters;  // suppress unused variable warning
  constexpr auto kBatchSize = 4;

  /// Get AKS System Manager instance
  this->sysMan_ = AKS::SysManagerExt::getGlobal();

  this->batch_size_ = kBatchSize;
}

constexpr auto kImageWidth = 1920;
constexpr auto kImageHeight = 1080;
constexpr auto kImageChannels = 3;
constexpr auto kImageSize = kImageWidth * kImageHeight * kImageChannels;

size_t AksDetectStream::doAllocate(size_t num) {
  constexpr auto kBufferNum = 10U;
  constexpr auto kBufferSize = 128;
  size_t buffer_num =
    static_cast<int>(num) == kNumBufferAuto ? kBufferNum : num;
  VectorBuffer::allocate(this->input_buffers_, buffer_num, kBufferSize,
                         DataType::STRING);
  VectorBuffer::allocate(this->output_buffers_, buffer_num,
                         kImageSize * this->batch_size_, DataType::INT8);
  return buffer_num;
}

void AksDetectStream::doAcquire(RequestParameters* parameters) {
  auto kPath =
    std::string("${AKS_ROOT}/graph_zoo/graph_yolov3_u200_u250_proteus.json");
  std::string kGraphName = "yolov3";

  auto path = kPath;
  if (parameters->has("aks_graph")) {
    path = parameters->get<std::string>("aks_graph");
  }
  autoExpandEnvironmentVariables(path);
  this->sysMan_->loadGraphs(path);

  auto graph_name = kGraphName;
  if (parameters->has("aks_graph_name")) {
    graph_name = parameters->get<std::string>("aks_graph_name");
  }
  this->graph_ = this->sysMan_->getGraph(graph_name);

  this->metadata_.addInputTensor(
    "input", types::DataType::INT8,
    {this->batch_size_, kImageHeight, kImageWidth, kImageChannels});
  // TODO(varunsh): what should we return here?
  this->metadata_.addOutputTensor("output", types::DataType::UINT32, {0});
  this->metadata_.setName(graph_name);
}

void AksDetectStream::doRun(BatchPtrQueue* input_queue) {
  std::shared_ptr<InferenceRequest> req;
  setThreadName("AksDetectStream");

  while (true) {
    BatchPtr batch;
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }

    SPDLOG_LOGGER_INFO(this->logger_, "Got request in AksDetectStream");
    for (unsigned int k = 0; k < batch->requests->size(); k++) {
      auto& req = batch->requests->at(k);
#ifdef PROTEUS_ENABLE_TRACING
      auto& trace = batch->traces.at(k);
      trace->startSpan("aks_detect_stream");
#endif
      auto inputs = req->getInputs();
      auto outputs = req->getOutputs();
      auto key = req->getParameters()->get<std::string>("key");
      for (auto& input : inputs) {
        auto* input_buffer = input.getData();

        auto* idata = static_cast<char*>(input_buffer);

        cv::VideoCapture cap(idata);  // open the video file
        if (!cap.isOpened()) {        // check if we succeeded
          const char* error = "Cannot open video file";
          SPDLOG_LOGGER_ERROR(this->logger_, error);
          req->runCallbackError(error);
          continue;
        }

        InferenceResponse resp;
        resp.setID(req->getID());
        resp.setModel("aks_detect_stream");

        // contains the number of frames in the video;
        auto count = static_cast<size_t>(
          cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_COUNT));
        if (input.getParameters()->has("count")) {
          auto requested_count = input.getParameters()->get<int32_t>("count");
          count = std::min(count, static_cast<size_t>(requested_count));
        }
        double fps = cap.get(cv::VideoCaptureProperties::CAP_PROP_FPS);
        auto video_width =
          cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH);
        auto video_height =
          cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT);

        InferenceResponseOutput output;
        output.setName("key");
        output.setDatatype(types::DataType::STRING);
        std::string metadata = "[" + std::to_string(video_width) + "," +
                               std::to_string(video_height) + "]";
        auto message = constructMessage(key, std::to_string(fps), metadata);
        output.setData(&message);
        output.setShape({message.size()});
        resp.addOutput(output);
        req->runCallback(resp);

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

#ifdef PROTEUS_ENABLE_TRACING
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
            std::string encoded = base64_encode(enc_msg, buf.size());
            frames.push("data:image/jpg;base64," + encoded);
          }
          SPDLOG_LOGGER_INFO(this->logger_, "Enqueuing in " + key);
          futures.push(
            this->sysMan_->enqueueJob(this->graph_, "", std::move(v), nullptr));
#ifdef PROTEUS_ENABLE_TRACING
          trace->endSpan();
#endif
          auto status = futures.front().wait_for(std::chrono::seconds(0));
          if (status == std::future_status::ready) {
            std::vector<std::unique_ptr<vart::TensorBuffer>> outDD =
              futures.front().get();
            futures.pop();
            auto* topKData = reinterpret_cast<float*>(outDD[0]->data().first);
            auto shape = outDD[0]->get_tensor()->get_shape();
            std::vector<std::string> labels;
            labels.reserve(this->batch_size_);
            for (unsigned int j = 0; j < this->batch_size_; j++) {
              labels.emplace_back("[");
            }
            for (int i = 0; i < shape[0] * shape[1]; i += 7) {
              auto batch_id = static_cast<int>(topKData[i]);
              auto class_id = std::to_string(topKData[i + 1]);
              auto score = std::to_string(topKData[i + 2]);
              auto x = std::to_string(topKData[i + 3]);
              auto y = std::to_string(topKData[i + 4]);
              auto w = std::to_string(topKData[i + 5]);
              auto h = std::to_string(topKData[i + 6]);
              labels[batch_id] += R"({"fill": false, "box": [)" + x + "," + y +
                                  "," + w + "," + h + R"(], "label": ")" +
                                  class_id + "\"},";
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
              output.setDatatype(types::DataType::STRING);
              auto message = constructMessage(key, frames.front(), labels[j]);
              output.setData(&message);
              output.setShape({message.size()});
              resp.addOutput(output);
              req->runCallback(resp);
              frames.pop();
            }
          }
        }
        while (!futures.empty()) {
          std::vector<std::unique_ptr<vart::TensorBuffer>> outDD =
            futures.front().get();
          SPDLOG_LOGGER_INFO(this->logger_, "Got future with key " + key);
          futures.pop();
          auto* topKData = reinterpret_cast<float*>(outDD[0]->data().first);
          auto shape = outDD[0]->get_tensor()->get_shape();
          std::vector<std::string> labels;
          labels.reserve(this->batch_size_);
          for (unsigned int j = 0; j < this->batch_size_; j++) {
            labels.emplace_back("[");
          }
          for (int i = 0; i < shape[0] * shape[1]; i += 7) {
            auto batch_id = static_cast<int>(topKData[i]);
            auto class_id = std::to_string(topKData[i + 1]);
            auto score = std::to_string(topKData[i + 2]);
            auto x = std::to_string(topKData[i + 3]);
            auto y = std::to_string(topKData[i + 4]);
            auto w = std::to_string(topKData[i + 5]);
            auto h = std::to_string(topKData[i + 6]);
            labels[batch_id] += R"({"fill": false, "box": [)" + x + "," + y +
                                "," + w + "," + h + R"(], "label": ")" +
                                class_id + "\"},";
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
            output.setDatatype(types::DataType::STRING);
            auto message = constructMessage(key, frames.front(), labels[j]);
            output.setData(&message);
            output.setShape({message.size()});
            resp.addOutput(output);
            req->runCallback(resp);
            frames.pop();
          }
        }
      }
    }
    this->returnBuffers(std::move(batch->input_buffers),
                        std::move(batch->output_buffers));
    SPDLOG_LOGGER_DEBUG(this->logger_, "Returned buffers");
  }
  SPDLOG_LOGGER_INFO(this->logger_, "AksDetectStream ending");
}

void AksDetectStream::doRelease() {}
void AksDetectStream::doDeallocate() {}
void AksDetectStream::doDestroy() {}

}  // namespace workers

}  // namespace proteus

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
proteus::workers::Worker* getWorker() {
  return new proteus::workers::AksDetectStream("AksDetectStream", "AKS");
}
}  // extern C
