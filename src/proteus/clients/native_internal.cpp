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

#include "proteus/clients/native_internal.hpp"

#include <algorithm>   // for fill
#include <cstddef>     // for size_t, byte
#include <cstdint>     // for uint64_t
#include <cstring>     // for memcpy
#include <functional>  // for _Bind_helper<>::type, _Placeh...
#include <numeric>     // for accumulate
#include <string>      // for string
#include <utility>     // for move

#include "proteus/buffers/buffer.hpp"       // for Buffer
#include "proteus/core/data_types.hpp"      // for getSize
#include "proteus/observation/logging.hpp"  // for Logger

namespace proteus {
template <typename T>
class InferenceRequestBuilder;

template <typename T>
class InferenceRequestInputBuilder;
}  // namespace proteus

namespace proteus {

template <>
class InferenceRequestInputBuilder<InferenceRequestInput> {
 public:
  static InferenceRequestInput build(const InferenceRequestInput &req,
                                     Buffer *input_buffer, size_t offset) {
    InferenceRequestInput input;
    input.data_ = req.data_;
    input.shared_data_ = nullptr;
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
  static InferenceRequestPtr build(
    const InferenceRequest &req, size_t &buffer_index,
    const std::vector<BufferRawPtrs> &input_buffers,
    std::vector<size_t> &input_offsets,
    const std::vector<BufferRawPtrs> &output_buffers,
    std::vector<size_t> &output_offsets, const size_t &batch_size,
    size_t &batch_offset) {
    auto request = std::make_shared<InferenceRequest>();

    request->id_ = req.id_;
    request->parameters_ = req.parameters_;
    request->callback_ = nullptr;

    auto buffer_index_backup = buffer_index;
    auto batch_offset_backup = batch_offset;

    for (const auto &input : req.inputs_) {
      const auto &buffers = input_buffers[buffer_index];
      for (auto &buffer : buffers) {
        auto &offset = input_offsets[buffer_index];

        request->inputs_.push_back(InputBuilder::build(input, buffer, offset));
        const auto &last_input = request->inputs_.back();
        offset += (last_input.getSize() * last_input.getDatatype().size());
      }
      batch_offset++;
      if (batch_offset == batch_size) {
        batch_offset = 0;
        buffer_index++;
        // std::fill(input_offsets.begin(), input_offsets.end(), 0);
      }
    }

    // TODO(varunsh): output_offset is currently ignored! The size of the output
    // needs to come from the worker but we have no such information.
    buffer_index = buffer_index_backup;
    batch_offset = batch_offset_backup;
    if (!req.outputs_.empty()) {
      for (const auto &output : req.outputs_) {
        const auto &buffers = output_buffers[buffer_index];
        for (auto &buffer : buffers) {
          const auto &offset = output_offsets[buffer_index];

          request->outputs_.emplace_back(output);
          request->outputs_.back().setData(
            static_cast<std::byte *>(buffer->data()) + offset);
        }
        batch_offset++;
        if (batch_offset == batch_size) {
          batch_offset = 0;
          buffer_index++;
          std::fill(output_offsets.begin(), output_offsets.end(), 0);
        }
      }
    } else {
      for (const auto &input : req.inputs_) {
        (void)input;  // suppress unused variable warning
        const auto &buffers = output_buffers[buffer_index];
        for (auto &buffer : buffers) {
          const auto &offset = output_offsets[buffer_index];

          request->outputs_.emplace_back();
          request->outputs_.back().setData(
            static_cast<std::byte *>(buffer->data()) + offset);
        }
        batch_offset++;
        if (batch_offset == batch_size) {
          batch_offset = 0;
          buffer_index++;
          std::fill(output_offsets.begin(), output_offsets.end(), 0);
        }
      }
    }

    return request;
  }
};

using RequestBuilder = InferenceRequestBuilder<InferenceRequest>;

CppNativeApi::CppNativeApi(InferenceRequest request)
  : request_(std::move(request)) {
  this->promise_ = std::make_unique<std::promise<proteus::InferenceResponse>>();
}

size_t CppNativeApi::getInputSize() { return this->request_.getInputSize(); }

std::promise<proteus::InferenceResponse> *CppNativeApi::getPromise() {
  return this->promise_.get();
}

// void cppCallback(const InferenceResponsePromisePtr &promise,
//                  const InferenceResponse &response) {
//   promise->set_value(response);
// }

std::shared_ptr<InferenceRequest> CppNativeApi::getRequest(
  size_t &buffer_index, const std::vector<BufferRawPtrs> &input_buffers,
  std::vector<size_t> &input_offsets,
  const std::vector<BufferRawPtrs> &output_buffers,
  std::vector<size_t> &output_offsets, const size_t &batch_size,
  size_t &batch_offset) {
  auto request = RequestBuilder::build(
    this->request_, buffer_index, input_buffers, input_offsets, output_buffers,
    output_offsets, batch_size, batch_offset);
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

}  // namespace proteus
