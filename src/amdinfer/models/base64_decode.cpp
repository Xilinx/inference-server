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
 * @brief Implements the base64_codec model
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

extern "C" {

std::vector<amdinfer::Tensor> getInputs() {
  // intentionally return an empty shape. This forces the worker to use
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

  auto new_batch = batch->propagate();
  const auto batch_size = batch->size();

  // const auto data_size = amdinfer::DataType("Uint8").size();
  // std::vector<amdinfer::BufferPtr> input_buffers;
  // input_buffers.emplace_back(std::make_unique<amdinfer::VectorBuffer>(
  //   kMaxImageSize * batch_size * data_size));

  std::vector<cv::Mat> decoded_images;
  size_t max_decoded_size = 0;
  const auto channels = 3;  // assuming 3 channels
  for (unsigned int j = 0; j < batch_size; j++) {
    const auto& req = batch->getRequest(j);
#ifdef AMDINFER_ENABLE_TRACING
    const auto& trace = batch->getTrace(j);
    trace->startSpan("base64_decode");
#endif
    const auto& inputs = req->getInputs();
    if (inputs.size() != 1) {
      req->runCallbackError("Only one input tensor should be present");
      continue;
    }
    auto& input = inputs.at(0);
    const auto& input_shape = input.getShape();
    auto input_size = amdinfer::util::containerProduct(input_shape);

    auto new_request = req->propagate();

    auto* input_data = static_cast<char*>(input.getData());
    auto decoded_str = amdinfer::util::base64Decode(input_data, input_size);
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

    const size_t decoded_size = img.rows * img.cols * channels;
    if (decoded_size > max_decoded_size) {
      max_decoded_size = decoded_size;
    }

    std::vector<uint64_t> shape{static_cast<uint64_t>(img.rows),
                                static_cast<uint64_t>(img.cols), 3};

    new_request->addInputTensor(nullptr, shape, amdinfer::DataType::Uint8,
                                "output");
    decoded_images.emplace_back(std::move(img));

    new_batch->addRequest(new_request);

#ifdef AMDINFER_ENABLE_TRACING
    trace->endSpan();
#endif
  }

  const auto data_size = amdinfer::DataType("Uint8").size();
  std::vector<amdinfer::BufferPtr> input_buffers;
  input_buffers.emplace_back(std::make_unique<amdinfer::VectorBuffer>(
    max_decoded_size * batch_size * data_size));

  for (auto j = 0U; j < batch_size; j++) {
    auto* data_ptr =
      input_buffers.at(0)->data(j * max_decoded_size * data_size);
    const auto& req = new_batch->getRequest(j);
    req->setInputTensorData(j, data_ptr);
    const auto& data = decoded_images.at(j);
    const auto decoded_size = data.rows * data.cols * channels;
    amdinfer::util::copy(data.data, static_cast<std::byte*>(data_ptr),
                         decoded_size);
  }

  new_batch->setBuffers(std::move(input_buffers), {});

  return new_batch;
}

}  // extern "C"
