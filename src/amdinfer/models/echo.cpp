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
 * @brief Implements the echo model
 */

#include "amdinfer/batching/batch.hpp"
#include "amdinfer/core/parameters.hpp"
#include "amdinfer/core/predict_api.hpp"
#include "amdinfer/observation/logging.hpp"
#include "amdinfer/observation/metrics.hpp"
#include "amdinfer/observation/tracing.hpp"
#include "amdinfer/util/memory.hpp"
#include "amdinfer/util/timer.hpp"

extern "C" {

std::vector<amdinfer::InferenceRequestInput> getInputs() {
  std::vector<amdinfer::InferenceRequestInput> input_tensors;
  std::vector<size_t> shape = {1};
  input_tensors.emplace_back(nullptr, shape, amdinfer::DataType::Uint32, "");
  return input_tensors;
}

std::vector<amdinfer::InferenceRequestInput> getOutputs() {
  std::vector<amdinfer::InferenceRequestInput> output_tensors;
  std::vector<size_t> shape = {1};
  output_tensors.emplace_back(nullptr, shape, amdinfer::DataType::Uint32, "");
  return output_tensors;
}

void run(amdinfer::Batch* batch, amdinfer::Batch* new_batch) {
  amdinfer::Logger logger{amdinfer::Loggers::Server};

  const auto batch_size = batch->size();
  for (unsigned int j = 0; j < batch_size; j++) {
    const auto& req = batch->getRequest(j);
#ifdef AMDINFER_ENABLE_TRACING
    auto& trace = batch->getTrace(j);
    trace->startSpan("echo");
#endif
    const auto& new_request = new_batch->getRequest(j);
    new_request->setCallback(req->getCallback());

    // new_request->setID(req->getID());
    const auto& inputs = req->getInputs();
    // auto outputs = req->getOutputs();
    const auto inputs_size = inputs.size();

    const auto& new_inputs = new_request->getInputs();
    for (auto i = 0U; i < inputs_size; ++i) {
      auto* input_buffer = inputs[i].getData();
      // std::byte* output_buffer = outputs[i].getData();
      // auto* input_buffer = dynamic_cast<VectorBuffer*>(input_ptr);
      // auto* output_buffer = dynamic_cast<VectorBuffer*>(output_ptr);

      uint32_t value = *static_cast<uint32_t*>(input_buffer);

      // this is my operation: add one to the read argument. While this can't
      // raise an exception, if exceptions can happen, they should be handled
      try {
        value++;
      } catch (const std::exception& e) {
        AMDINFER_LOG_ERROR(logger, e.what());
        req->runCallbackError("Something went wrong");
        continue;
      }

      // amdinfer::InferenceRequestInput new_input;
      // new_input.setDatatype(amdinfer::DataType::Uint32);
      // new_input.setName(inputs[i].getName());
      // new_input.setShape({1});
      // new_input.setParameters(inputs[i].getParameters());

      amdinfer::util::copy(value,
                           static_cast<std::byte*>(new_inputs.at(i).getData()));

      // std::vector<std::byte> buffer;
      // buffer.resize(sizeof(uint32_t));
      // memcpy(buffer.data(), &value, sizeof(uint32_t));
      // output.setData(std::move(buffer));
      // resp.addOutput(output);
    }

#ifdef AMDINFER_ENABLE_TRACING
    trace->endSpan();
    new_batch->addTrace(std::move(trace));
#endif

#ifdef AMDINFER_ENABLE_METRICS
    new_batch->addTime(batch->getTime(j));
#endif
  }
}

}  // extern "C"
