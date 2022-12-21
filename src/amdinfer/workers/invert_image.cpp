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
 * @brief Implements the InvertImage worker
 */

#include <chrono>                 // for microseconds, duration_...
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

#include "amdinfer/batching/batcher.hpp"       // for Batch, BatchPtrQueue
#include "amdinfer/buffers/vector_buffer.hpp"  // for VectorBuffer
#include "amdinfer/build_options.hpp"          // for AMDINFER_ENABLE_TRACING
#include "amdinfer/core/data_types.hpp"        // for DataType, DataType::Uint8
#include "amdinfer/core/predict_api.hpp"       // for InferenceRequest, Infer...
#include "amdinfer/declarations.hpp"           // for BufferPtr, InferenceRes...
#include "amdinfer/observation/logging.hpp"    // for Logger
#include "amdinfer/observation/metrics.hpp"    // for Metrics
#include "amdinfer/observation/tracing.hpp"    // for startFollowSpan, SpanPtr
#include "amdinfer/util/base64.hpp"            // for base64_decode, base64_e...
#include "amdinfer/util/thread.hpp"            // for setThreadName
#include "amdinfer/workers/worker.hpp"         // for Worker

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

namespace amdinfer {

namespace workers {

/**
 * @brief The InvertImage worker is a simple worker that accepts an array
 * representing an image and adds one to each pixel. It accepts multiple input
 * tensors and returns the corresponding number of output tensors.
 *
 */
class InvertImage : public Worker {
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
                         kBufferSize * this->batch_size_, DataType::Uint8);
  VectorBuffer::allocate(this->output_buffers_, buffer_num,
                         kBufferSize * this->batch_size_, DataType::Uint8);
  return buffer_num;
}

void InvertImage::doAcquire(RequestParameters* parameters) {
  (void)parameters;  // suppress unused variable warning

  this->metadata_.addInputTensor("input", DataType::Uint8,
                                 {this->batch_size_, 1080, 1920, 3});
  this->metadata_.addOutputTensor("output", DataType::Uint32,
                                  {this->batch_size_, 1080, 1920, 3});
}

void InvertImage::doRun(BatchPtrQueue* input_queue) {
  util::setThreadName("InvertImage");
#ifdef AMDINFER_ENABLE_LOGGING
  const auto& logger = this->getLogger();
#endif

  while (true) {
    BatchPtr batch;
    input_queue->wait_dequeue(batch);
    if (batch == nullptr) {
      break;
    }

    AMDINFER_LOG_INFO(logger, "Got request in InvertImage");
    for (unsigned int j = 0; j < batch->size(); j++) {
      const auto& req = batch->getRequest(j);
#ifdef AMDINFER_ENABLE_TRACING
      const auto& trace = batch->getTrace(j);
      trace->startSpan("InvertImage");
#endif
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
        if (input_dtype == DataType::Uint8) {
          // Output will have the same shape as input
          output.setShape(input_shape);

          if (input_shape.size() == 3 && input_shape[2] == 3) {
            invert<uint8_t*, false>(input_buffer, output_buffer, input_size);
          } else {
            invert<uint8_t*, true>(input_buffer, output_buffer, input_size);
          }

          auto* output_data = static_cast<uint8_t*>(output_buffer);

          std::vector<std::byte> buffer;
          buffer.resize(input_size);
          memcpy(buffer.data(), output_data, input_size);
          output.setData(std::move(buffer));
          output.setDatatype(DataType::Uint8);
        } else if (input_dtype == DataType::String) {
          auto* idata = static_cast<char*>(input_buffer);
          auto decoded_str = util::base64_decode(idata, input_size);
          std::vector<char> data(decoded_str.begin(), decoded_str.end());
          cv::Mat img;
          try {
            img = cv::imdecode(data, cv::IMREAD_UNCHANGED);
          } catch (const cv::Exception& e) {
            AMDINFER_LOG_ERROR(logger, e.what());
            req->runCallbackError("Failed to decode base64 image data");
            continue;
          }

          if (img.empty()) {
            const char* error = "Decoded image is empty";
            AMDINFER_LOG_ERROR(logger, error);
            req->runCallbackError(error);
            continue;
          }
          cv::bitwise_not(img, img);
          std::vector<unsigned char> buf;
          cv::imencode(".jpg", img, buf);
          const auto* enc_msg = reinterpret_cast<const char*>(buf.data());
          auto encoded = util::base64_encode(enc_msg, buf.size());
          std::vector<std::byte> buffer;
          buffer.resize(encoded.size());
          memcpy(buffer.data(), encoded.data(), encoded.length());
          output.setData(std::move(buffer));
          output.setDatatype(DataType::String);
          output.setShape({encoded.size()});
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
#ifdef AMDINFER_ENABLE_METRICS
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - batch->getTime(j));
      Metrics::getInstance().observeSummary(MetricSummaryIDs::kRequestLatency,
                                            duration.count());
#endif
#ifdef AMDINFER_ENABLE_TRACING
      auto context = trace->propagate();
      resp.setContext(std::move(context));
#endif
      req->runCallbackOnce(resp);
    }
  }
  AMDINFER_LOG_INFO(logger, "InvertImage ending");
}

void InvertImage::doRelease() {}
void InvertImage::doDeallocate() {}
void InvertImage::doDestroy() {}

}  // namespace workers

}  // namespace amdinfer

extern "C" {
// using smart pointer here may cause problems inside shared object so managing
// manually
amdinfer::workers::Worker* getWorker() {
  return new amdinfer::workers::InvertImage("InvertImage", "CPU");
}
}  // extern C
