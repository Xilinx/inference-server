// Copyright 2023 Advanced Micro Devices, Inc.
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
 * @brief Implements the echo_multi model
 */

#include <opencv2/core.hpp>       // for bitwise_not, Mat
#include <opencv2/imgcodecs.hpp>  // for imdecode, imencode, IMRE...

#include "amdinfer/batching/batch.hpp"
#include "amdinfer/buffers/vector.hpp"
#include "amdinfer/core/inference_request.hpp"
#include "amdinfer/core/inference_response.hpp"
#include "amdinfer/core/parameters.hpp"
#include "amdinfer/observation/logging.hpp"
#include "amdinfer/observation/metrics.hpp"
#include "amdinfer/observation/tracing.hpp"
#include "amdinfer/util/base64.hpp"  // for base64_decode, base64_e...
#include "amdinfer/util/containers.hpp"
#include "amdinfer/util/memory.hpp"
#include "amdinfer/util/timer.hpp"

/// Invert the pixel color. Assumes RGB in some order with optional alpha
template <typename T, bool kAlphaPresent>
void invert(void* ibuf, void* obuf, uint64_t size) {
  static_assert(std::is_pointer_v<T>, "T must be a pointer type");
  auto* idata = static_cast<T>(ibuf);
  auto* odata = static_cast<T>(obuf);
  static_assert(sizeof(idata[0]) < sizeof(uint64_t), "T must be <8 bytes");

  // mask to get the largest value. E.g. for uint8_t, mask will be 255.
  const auto mask = (1ULL << (sizeof(idata[0]) * CHAR_BIT)) - 1;
  const uint64_t incr = kAlphaPresent ? 4 : 3;
  for (uint64_t i = 0; i < size; i += incr) {
    odata[i] = mask - idata[i];
    odata[i + 1] = mask - idata[i + 1];
    odata[i + 2] = mask - idata[i + 2];
    if constexpr (kAlphaPresent) {
      odata[i + 3] = idata[i + 3];
    }
  }
}

extern "C" {

std::vector<amdinfer::Tensor> getInputs() {
  // intentionally return an empty shape, indicating an unknown output tensor
  // size even though it is known in this case. This forces the worker to use
  // different logic to invoke the run method.
  return {};
}

std::vector<amdinfer::Tensor> getOutputs() { return {}; }

// Support up to Full HD
const auto kMaxImageHeight = 1080;
const auto kMaxImageWidth = 1920;
const auto kMaxImageChannels = 3;
const auto kMaxImageSize = kMaxImageHeight * kMaxImageWidth * kMaxImageChannels;

amdinfer::BatchPtr run(amdinfer::Batch* batch) {
  amdinfer::Logger logger{amdinfer::Loggers::Server};

  auto new_batch = std::make_unique<amdinfer::Batch>();
  const auto batch_size = batch->size();
  const auto data_size = amdinfer::DataType("Uint8").size();

  std::vector<amdinfer::BufferPtr> input_buffers;
  input_buffers.emplace_back(std::make_unique<amdinfer::VectorBuffer>(
    kMaxImageSize * batch_size * data_size));

  for (unsigned int j = 0; j < batch_size; j++) {
    const auto& req = batch->getRequest(j);
#ifdef AMDINFER_ENABLE_TRACING
    const auto& trace = batch->getTrace(j);
    trace->startSpan("invert_image");
#endif
    const auto& inputs = req->getInputs();
    auto input_shape = inputs[0].getShape();

    auto input_size = amdinfer::util::containerProduct(input_shape);
    auto input_datatype = inputs[0].getDatatype();

    auto new_request = req->propagate();
    auto* data_ptr = input_buffers.at(0)->data(j * kMaxImageSize * data_size);
    if (input_datatype == amdinfer::DataType::Uint8) {
      new_request->addInputTensor(data_ptr, input_shape,
                                  amdinfer::DataType::Uint8, "output");
      invert<uint8_t*, false>(inputs[0].getData(), data_ptr, input_size);
    } else {
      auto* idata = static_cast<char*>(inputs[0].getData());
      auto decoded_str = amdinfer::util::base64Decode(idata, input_size);
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
      auto encoded = amdinfer::util::base64Encode(enc_msg, buf.size());
      amdinfer::util::copy(encoded.data(), static_cast<std::byte*>(data_ptr),
                           encoded.size());
      new_request->addInputTensor(data_ptr, {encoded.size()},
                                  amdinfer::DataType::String, "output");
    }

    new_batch->addRequest(new_request);
    new_batch->setModel(j, "invert_image");

#ifdef AMDINFER_ENABLE_TRACING
    trace->endSpan();
#endif

#ifdef AMDINFER_ENABLE_METRICS
    new_batch->addTime(batch->getTime(j));
#endif
  }

  new_batch->setBuffers(std::move(input_buffers), {});

  return new_batch;
}

}  // extern "C"
