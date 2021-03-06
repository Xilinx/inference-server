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
 * @brief Implements the InvertVideo worker
 */

#include <cstddef>                // for size_t
#include <cstdint>                // for int32_t
#include <memory>                 // for allocator, unique_ptr
#include <opencv2/core.hpp>       // for bitwise_not, Mat
#include <opencv2/imgcodecs.hpp>  // for imencode
#include <opencv2/videoio.hpp>    // for VideoCapture, CV_CAP_PRO...
#include <string>                 // for string, operator+, char_...
#include <thread>                 // for thread
#include <utility>                // for move
#include <vector>                 // for vector

#include "proteus/batching/batcher.hpp"       // for Batch, BatchPtrQueue
#include "proteus/buffers/vector_buffer.hpp"  // for VectorBuffer
#include "proteus/build_options.hpp"          // for PROTEUS_ENABLE_TRACING
#include "proteus/core/data_types.hpp"        // for DataType, DataType::STRING
#include "proteus/core/predict_api.hpp"       // for InferenceResponse, Infer...
#include "proteus/helpers/base64.hpp"         // for base64_encode
#include "proteus/helpers/declarations.hpp"   // for BufferPtr, InferenceResp...
#include "proteus/helpers/thread.hpp"         // for setThreadName
#include "proteus/observation/logging.hpp"    // for Logger
#include "proteus/observation/tracing.hpp"    // for startFollowSpan, SpanPtr
#include "proteus/workers/worker.hpp"         // for Worker

namespace proteus {

std::string constructMessage(const std::string& key, const std::string& data) {
  const std::string labels = R"([])";
  return R"({"key": ")" + key + R"(", "data": {"img": ")" + data +
         R"(", "labels": )" + labels + "}}";
}

namespace workers {

/**
 * @brief The InvertVideo worker is a simple worker that accepts an path to a
 * video and sends the inverted frames back to the client over a websocket.
 *
 */
class InvertVideo : public Worker {
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
};

std::thread InvertVideo::spawn(BatchPtrQueue* input_queue) {
  return std::thread(&InvertVideo::run, this, input_queue);
}

void InvertVideo::doInit(RequestParameters* parameters) {
  constexpr auto kMaxBufferNum = 50;
  constexpr auto kBatchSize = 1;

  auto max_buffer_num = kMaxBufferNum;
  if (parameters->has("max_buffer_num")) {
    max_buffer_num = parameters->get<int32_t>("max_buffer_num");
  }
  this->max_buffer_num_ = max_buffer_num;

  this->batch_size_ = kBatchSize;
}

size_t InvertVideo::doAllocate(size_t num) {
  constexpr auto kBufferNum = 10U;
  constexpr auto kBufferSize = 128;
  size_t buffer_num =
    static_cast<int>(num) == kNumBufferAuto ? kBufferNum : num;
  VectorBuffer::allocate(this->input_buffers_, buffer_num, kBufferSize,
                         DataType::STRING);
  VectorBuffer::allocate(this->output_buffers_, buffer_num, 1920 * 1080 * 3,
                         DataType::INT8);
  return buffer_num;
}

void InvertVideo::doAcquire(RequestParameters* parameters) {
  (void)parameters;  // suppress unused variable warning

  this->metadata_.addInputTensor("input", DataType::STRING, {128});
  // TODO(varunsh): output is variable
  this->metadata_.addOutputTensor("output", DataType::INT8, {1080, 1920, 3});
}

void InvertVideo::doRun(BatchPtrQueue* input_queue) {
  std::shared_ptr<InferenceRequest> req;
  setThreadName("InvertVideo");
#ifdef PROTEUS_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  while (true) {
    BatchPtr batch;
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }

    PROTEUS_LOG_INFO(logger, "Got request in InvertVideo");
    for (unsigned int j = 0; j < batch->requests->size(); j++) {
      auto& req = batch->requests->at(j);
#ifdef PROTEUS_ENABLE_TRACING
      auto& trace = batch->traces.at(j);
      trace->startSpan("InvertVideo");
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
          PROTEUS_LOG_ERROR(logger, error);
          req->runCallbackError(error);
          continue;
        }

        InferenceResponse resp;
        resp.setID(req->getID());
        resp.setModel("invert_video");

        // contains the number of frames in the video;
        auto count = static_cast<int32_t>(
          cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_COUNT));
        if (input.getParameters()->has("count")) {
          count = input.getParameters()->get<int32_t>("count");
        }
        double fps = cap.get(cv::VideoCaptureProperties::CAP_PROP_FPS);

        InferenceResponseOutput output;
        output.setName("key");
        output.setDatatype(DataType::STRING);
        auto message = constructMessage(key, std::to_string(fps));
        output.setData(&message);
        output.setShape({message.size()});
        resp.addOutput(output);
        req->runCallback(resp);
        for (int frameNum = 0; frameNum < count; frameNum++) {
          cv::Mat frame;
          cap >> frame;  // get the next frame from video
          if (frame.empty()) {
            frameNum--;
            continue;
          }
          cv::bitwise_not(frame, frame);
          std::vector<unsigned char> buf;
          cv::imencode(".jpg", frame, buf);
          const auto* enc_msg = reinterpret_cast<const char*>(buf.data());
          std::string encoded =
            "data:image/jpg;base64," + base64_encode(enc_msg, buf.size());

          InferenceResponse resp;
          resp.setID(req->getID());
          resp.setModel("invert_video");

          InferenceResponseOutput output;
          output.setName("image");
          output.setDatatype(DataType::STRING);
          auto message = constructMessage(key, encoded);
          output.setData(&message);
          output.setShape({message.size()});
          resp.addOutput(output);
          req->runCallback(resp);
        }
      }
    }
    this->returnBuffers(std::move(batch->input_buffers),
                        std::move(batch->output_buffers));
    PROTEUS_LOG_DEBUG(logger, "Returned buffers");
  }
  // if (req != nullptr && req->getWsConn()->connected()) {
  //   req->getWsConn()->shutdown(drogon::CloseCode::kNormalClosure);
  // }
  PROTEUS_LOG_INFO(logger, "InvertVideo ending");
}

void InvertVideo::doRelease() {}
void InvertVideo::doDeallocate() {}
void InvertVideo::doDestroy() {}

}  // namespace workers

}  // namespace proteus

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
proteus::workers::Worker* getWorker() {
  return new proteus::workers::InvertVideo("InvertVideo", "CPU");
}
}  // extern C
