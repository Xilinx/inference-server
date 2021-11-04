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
 * @brief Implements the InvertImage worker
 */

#include <climits>                // for CHAR_BIT
#include <cstddef>                // for size_t, byte
#include <cstdint>                // for uint8_t, uint64_t, int32_t
#include <cstring>                // for memcpy
#include <functional>             // for multiplies, function
#include <memory>                 // for unique_ptr, allocator
#include <numeric>                // for accumulate
#include <opencv2/core.hpp>       // for bitwise_not, Mat
#include <opencv2/imgcodecs.hpp>  // for imdecode, imencode, IMRE...
#include <string>                 // for string, basic_string
#include <thread>                 // for thread
#include <type_traits>            // for is_pointer
#include <utility>                // for move
#include <vector>                 // for vector

#include "proteus/batching/batcher.hpp"       // for Batch, BatchPtrQueue
#include "proteus/buffers/vector_buffer.hpp"  // for VectorBuffer
#include "proteus/build_options.hpp"          // for PROTEUS_ENABLE_TRACING
#include "proteus/core/data_types.hpp"        // for DataType, DataType::UINT8
#include "proteus/core/predict_api.hpp"       // for InferenceRequest, Infere...
#include "proteus/helpers/base64.hpp"         // for base64_decode, base64_en...
#include "proteus/helpers/declarations.hpp"   // for BufferPtr, InferenceResp...
#include "proteus/helpers/thread.hpp"         // for setThreadName
#include "proteus/observation/logging.hpp"    // for SPDLOG_LOGGER_INFO, SPDL...
#include "proteus/observation/tracing.hpp"    // for startFollowSpan, SpanPtr
#include "proteus/workers/worker.hpp"         // for Worker

namespace {

/// Invert the pixel color. Assumes RGB in some order with optional alpha
template <typename T, bool alphaPresent>
void invert(void* ibuf, void* obuf, uint64_t size) {
  static_assert(std::is_pointer<T>::value, "T must be a pointer type");
  auto* idata = static_cast<T>(ibuf);
  auto* odata = static_cast<T>(obuf);
  static_assert(sizeof(idata[0]) < sizeof(uint64_t), "T must be <8 bytes");

  // mask to get the largest value. E.g. for uint8_t, mask will be 255.
  constexpr auto mask = (1ULL << (sizeof(idata[0]) * CHAR_BIT)) - 1;
  constexpr uint64_t incr = alphaPresent ? 4 : 3;
  for (uint64_t i = 0; i < size; i += incr) {
    odata[i] = mask - idata[i];
    odata[i + 1] = mask - idata[i + 1];
    odata[i + 2] = mask - idata[i + 2];
    if constexpr (alphaPresent) {
      odata[i + 3] = idata[i + 3];
    }
  }
}

/// Reduce vector to a product of its elements
uint64_t reduce_mult(std::vector<uint64_t>& v) {
  return std::accumulate(v.begin(), v.end(), 1, std::multiplies<>());
}

}  // namespace

namespace proteus {

using types::DataType;

namespace workers {

/**
 * @brief The InvertImage worker is a simple worker that accepts an array
 * representing an image and adds one to each pixel. It accepts multiple input
 * tensors and returns the corresponding number of output tensors.
 *
 */
class InvertImage : public Worker {
 public:
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

std::thread InvertImage::spawn(BatchPtrQueue* input_queue) {
  return std::thread(&InvertImage::run, this, input_queue);
}

void InvertImage::doInit(RequestParameters* parameters) {
  constexpr auto kMaxBufferNum = 50;
  constexpr auto kBatchSize = 1;

  auto max_buffer_num = kMaxBufferNum;
  if (parameters->has("max_buffer_num")) {
    max_buffer_num = parameters->get<int32_t>("max_buffer_num");
  }
  this->max_buffer_num_ = max_buffer_num;

  auto batch_size = kBatchSize;
  if (parameters->has("batch_size")) {
    batch_size = parameters->get<int32_t>("batch_size");
  }
  this->batch_size_ = batch_size;
}

size_t InvertImage::doAllocate(size_t num) {
  constexpr auto kBufferNum = 10U;
  constexpr auto kBufferSize = 1920 * 1080 * 3;  // Support up to Full HD
  size_t buffer_num =
    static_cast<int>(num) == kNumBufferAuto ? kBufferNum : num;
  VectorBuffer::allocate(this->input_buffers_, buffer_num,
                         kBufferSize * this->batch_size_, DataType::UINT8);
  VectorBuffer::allocate(this->output_buffers_, buffer_num,
                         kBufferSize * this->batch_size_, DataType::UINT8);
  return buffer_num;
}

void InvertImage::doAcquire(RequestParameters* parameters) {
  (void)parameters;  // suppress unused variable warning
}

void InvertImage::doRun(BatchPtrQueue* input_queue) {
  std::shared_ptr<InferenceRequest> req;
  std::unique_ptr<Batch> batch;
  setThreadName("InvertImage");

  while (true) {
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }

    SPDLOG_LOGGER_INFO(this->logger_, "Got request in InvertImage");
#ifdef PROTEUS_ENABLE_TRACING
    auto span = startFollowSpan(batch->span.get(), "InvertImage");
#endif
    for (auto& req : *(batch->requests)) {
      InferenceResponse resp;
      resp.setID(req->getID());
      resp.setModel("invert_image");
      auto inputs = req->getInputs();
      auto outputs = req->getOutputs();
      for (unsigned int i = 0; i < inputs.size(); i++) {
        auto* input_buffer = inputs[i].getData();
        auto* output_buffer = outputs[i].getData();

        auto input_shape = inputs[i].getShape();

        auto input_size = reduce_mult(input_shape);
        auto input_dtype = inputs[i].getDatatype();

        // invert image, store in output
        InferenceResponseOutput output;
        if (input_dtype == DataType::UINT8) {
          // Output will have the same shape as input
          output.setShape(input_shape);

          if (input_shape.size() == 3 && input_shape[2] == 3) {
            invert<uint8_t*, false>(input_buffer, output_buffer, input_size);
          } else {
            invert<uint8_t*, true>(input_buffer, output_buffer, input_size);
          }

          auto* output_data = static_cast<uint8_t*>(output_buffer);

          auto buffer = std::make_shared<std::vector<uint8_t>>();
          buffer->reserve(input_size);
          memcpy(buffer->data(), output_data, input_size);
          auto my_data_cast = std::reinterpret_pointer_cast<std::byte>(buffer);
          output.setData(std::move(my_data_cast));
          output.setDatatype(types::DataType::UINT8);
        } else if (input_dtype == DataType::STRING) {
          auto* idata = static_cast<char*>(input_buffer);
          auto decoded_str = base64_decode(idata, input_size);
          std::vector<char> data(decoded_str.begin(), decoded_str.end());
          cv::Mat img = cv::imdecode(data, cv::IMREAD_UNCHANGED);
          if (img.empty()) {
            SPDLOG_LOGGER_WARN(this->logger_, "Wait, image is empty!");
          }
          cv::bitwise_not(img, img);
          std::vector<unsigned char> buf;
          cv::imencode(".jpg", img, buf);
          const auto* enc_msg = reinterpret_cast<const char*>(buf.data());
          auto encoded =
            std::make_shared<std::string>(base64_encode(enc_msg, buf.size()));
          auto my_data_cast = std::reinterpret_pointer_cast<std::byte>(encoded);
          output.setData(std::move(my_data_cast));
          output.setDatatype(types::DataType::STRING);
          output.setShape({buf.size()});
        }

        // if our output is explicitly named, use that name in response, or use
        // the input tensor's name if it's not defined.
        std::string output_name = outputs[i].getName();
        if (output_name.empty()) {
          output.setName(inputs[i].getName());
        } else {
          output.setName(output_name);
        }

        resp.addOutput(output);
      }
      req->getCallback()(resp);
    }
    this->returnBuffers(std::move(batch->input_buffers),
                        std::move(batch->output_buffers));
    SPDLOG_LOGGER_DEBUG(this->logger_, "Returned buffers");
  }
  SPDLOG_LOGGER_INFO(this->logger_, "InvertImage ending");
}

void InvertImage::doRelease() {}
void InvertImage::doDeallocate() {}
void InvertImage::doDestroy() {}

}  // namespace workers

}  // namespace proteus

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
proteus::workers::Worker* getWorker() {
  return new proteus::workers::InvertImage();
}
}  // extern C
