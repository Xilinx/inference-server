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

amdinfer::BatchPtr run(amdinfer::Batch* batch) {
  AMDINFER_IF_LOGGING(amdinfer::Logger logger{amdinfer::Loggers::Server});

  auto new_batch = batch->propagate();
  const auto batch_size = batch->size();

  std::vector<std::string> encoded_images;
  size_t max_encoded_size = 0;
  for (unsigned int j = 0; j < batch_size; j++) {
    const auto& req = batch->getRequest(j);
#ifdef AMDINFER_ENABLE_TRACING
    const auto& trace = batch->getTrace(j);
    trace->startSpan("base64_encode");
#endif
    const auto& inputs = req->getInputs();
    if (inputs.size() != 1) {
      req->runCallbackError("Only one input tensor should be present");
      continue;
    }
    const auto& input = inputs.at(0);
    auto input_shape = input.getShape();

    auto new_request = req->propagate();

    auto img =
      cv::Mat{static_cast<int>(input_shape[0]),
              static_cast<int>(input_shape[1]), CV_8UC3, input.getData()};
    std::vector<unsigned char> buf;
    cv::imencode(".jpg", img, buf);
    const auto* enc_msg = reinterpret_cast<const char*>(buf.data());
    auto encoded = amdinfer::util::base64Encode(enc_msg, buf.size());
    const auto encoded_size = encoded.size();
    if (encoded_size > max_encoded_size) {
      max_encoded_size = encoded_size;
    }

    new_request->addInputTensor(nullptr, {static_cast<int64_t>(encoded_size)},
                                amdinfer::DataType::Bytes, "output");
    encoded_images.emplace_back(std::move(encoded));

    new_batch->addRequest(new_request);

#ifdef AMDINFER_ENABLE_TRACING
    trace->endSpan();
#endif
  }

  const auto data_size = amdinfer::DataType("Uint8").size();
  std::vector<amdinfer::BufferPtr> input_buffers;
  input_buffers.emplace_back(std::make_unique<amdinfer::VectorBuffer>(
    max_encoded_size * batch_size * data_size));

  for (auto j = 0U; j < batch_size; j++) {
    auto* data_ptr =
      input_buffers.at(0)->data(j * max_encoded_size * data_size);
    const auto& req = new_batch->getRequest(j);
    req->setInputTensorData(j, data_ptr);
    const auto& data = encoded_images.at(j);
    amdinfer::util::copy(data.data(), static_cast<std::byte*>(data_ptr),
                         data.size());
  }

  new_batch->setBuffers(std::move(input_buffers), {});

  return new_batch;
}

}  // extern "C"
