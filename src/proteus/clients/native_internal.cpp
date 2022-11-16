// Copyright 2022 Xilinx Inc.
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
 * @brief Implements the internal objects used for the C++ API
 */

#include "amdinfer/clients/native_internal.hpp"

#include <cstddef>     // for byte, size_t
#include <cstdint>     // for uint64_t
#include <cstring>     // for memcpy
#include <functional>  // for multiplies
#include <numeric>     // for accumulate
#include <string>      // for string
#include <utility>     // for move

#include "amdinfer/buffers/buffer.hpp"       // for Buffer
#include "amdinfer/core/data_types.hpp"      // for DataType
#include "amdinfer/observation/logging.hpp"  // for Logger, PROTEUS_LOG_ERROR

namespace amdinfer {
template <typename T>
class InferenceRequestBuilder;

template <typename T>
class InferenceRequestInputBuilder;
}  // namespace amdinfer

namespace amdinfer {

template <>
class InferenceRequestInputBuilder<InferenceRequestInput> {
 public:
  static InferenceRequestInput build(const InferenceRequestInput &req,
                                     Buffer *input_buffer, size_t offset) {
    InferenceRequestInput input;
    input.data_ = req.data_;
    input.name_ = req.name_;
    input.shape_.reserve(req.shape_.size());
    input.shape_ = req.shape_;
    input.dataType_ = req.dataType_;
    input.parameters_ = req.parameters_;
    auto size = std::accumulate(input.shape_.begin(), input.shape_.end(), 1,
                                std::multiplies<>()) *
                input.dataType_.size();
    auto *dest = static_cast<std::byte *>(input_buffer->data()) + offset;
    memcpy(dest, req.data_, size);

    input.data_ = dest;
    return input;
  }
};

using InputBuilder = InferenceRequestInputBuilder<InferenceRequestInput>;

template <>
class InferenceRequestBuilder<InferenceRequest> {
 public:
  static InferenceRequestPtr build(const InferenceRequest &req,
                                   const BufferRawPtrs &input_buffers,
                                   std::vector<size_t> &input_offsets,
                                   const BufferRawPtrs &output_buffers,
                                   std::vector<size_t> &output_offsets) {
    auto request = std::make_shared<InferenceRequest>();

    request->id_ = req.id_;
    request->parameters_ = req.parameters_;
    request->callback_ = nullptr;

    for (const auto &input : req.inputs_) {
      const auto &buffers = input_buffers;
      auto index = 0;
      for (auto &buffer : buffers) {
        auto &offset = input_offsets[index];

        request->inputs_.push_back(InputBuilder::build(input, buffer, offset));
        const auto &last_input = request->inputs_.back();
        offset += (last_input.getSize() * last_input.getDatatype().size());
        index++;
      }
    }

    // TODO(varunsh): output_offset is currently ignored! The size of the output
    // needs to come from the worker but we have no such information.
    if (!req.outputs_.empty()) {
      for (const auto &output : req.outputs_) {
        const auto &buffers = output_buffers;
        auto index = 0;
        for (auto &buffer : buffers) {
          const auto &offset = output_offsets[index];

          request->outputs_.emplace_back(output);
          request->outputs_.back().setData(
            static_cast<std::byte *>(buffer->data()) + offset);
          index++;
        }
      }
    } else {
      for (const auto &input : req.inputs_) {
        (void)input;  // suppress unused variable warning
        const auto &buffers = output_buffers;
        auto index = 0;
        for (auto &buffer : buffers) {
          const auto &offset = output_offsets[index];

          request->outputs_.emplace_back();
          request->outputs_.back().setData(
            static_cast<std::byte *>(buffer->data()) + offset);
          index++;
        }
      }
    }

    return request;
  }
};

using RequestBuilder = InferenceRequestBuilder<InferenceRequest>;

CppNativeApi::CppNativeApi(InferenceRequest request)
  : request_(std::move(request)) {
  this->promise_ =
    std::make_unique<std::promise<amdinfer::InferenceResponse>>();
}

size_t CppNativeApi::getInputSize() { return this->request_.getInputSize(); }

std::promise<amdinfer::InferenceResponse> *CppNativeApi::getPromise() {
  return this->promise_.get();
}

// void cppCallback(const InferenceResponsePromisePtr &promise,
//                  const InferenceResponse &response) {
//   promise->set_value(response);
// }

std::shared_ptr<InferenceRequest> CppNativeApi::getRequest(
  const BufferRawPtrs &input_buffers, std::vector<size_t> &input_offsets,
  const BufferRawPtrs &output_buffers, std::vector<size_t> &output_offsets) {
  auto request =
    RequestBuilder::build(this->request_, input_buffers, input_offsets,
                          output_buffers, output_offsets);
  Callback callback =
    [promise = std::move(this->promise_)](const InferenceResponse &response) {
      promise->set_value(response);
    };
  request->setCallback(std::move(callback));
  return request;
}

void CppNativeApi::errorHandler(const std::exception &e) {
  PROTEUS_LOG_ERROR(this->getLogger(), e.what());
  this->getPromise()->set_value(InferenceResponse(e.what()));
}

}  // namespace amdinfer
