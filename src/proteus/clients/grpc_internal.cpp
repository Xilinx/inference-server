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
 * @brief Implements the internal objects used for gRPC
 */

#include "proteus/clients/grpc_internal.hpp"

#include <numeric>  // for accumulate

#include "predict_api.grpc.pb.h"
#include "proteus/buffers/buffer.hpp"
#include "proteus/core/predict_api_internal.hpp"
#include "proteus/servers/grpc_server.hpp"

namespace proteus {

using types::DataType;

RequestParametersPtr addParameters(
  const google::protobuf::Map<std::string, inference::InferParameter> &params) {
  auto parameters = std::make_shared<RequestParameters>();
  for (const auto &[key, value] : params) {
    if (value.has_bool_param()) {
      parameters->put(key, value.bool_param());
    } else if (value.has_int64_param()) {
      // TODO(varunsh): parameters should switch to uint64?
      parameters->put(key, static_cast<int>(value.int64_param()));
    } else if (value.has_double_param()) {
      parameters->put(key, value.double_param());
    } else {
      parameters->put(key, value.string_param());
    }
  }
  return parameters;
}

template <>
class InferenceRequestInputBuilder<
  inference::ModelInferRequest_InferInputTensor> {
 public:
  static InferenceRequestInput build(
    const inference::ModelInferRequest_InferInputTensor &req,
    Buffer *input_buffer, size_t offset) {
    InferenceRequestInput input;
    input.shared_data_ = nullptr;
    input.name_ = req.name();
    input.shape_.reserve(req.shape_size());
    for (const auto &index : req.shape()) {
      input.shape_.push_back(static_cast<size_t>(index));
    }
    input.dataType_ = types::mapStrToType(req.datatype());

    input.parameters_ = addParameters(req.parameters());

    auto size = std::accumulate(input.shape_.begin(), input.shape_.end(), 1,
                                std::multiplies<>()) *
                types::getSize(input.dataType_);
    auto *dest = static_cast<std::byte *>(input_buffer->data()) + offset;

    const auto tensor = req.contents();
    switch (input.getDatatype()) {
      case DataType::BOOL: {
        std::memcpy(dest, tensor.bool_contents().data(), size * sizeof(char));
        break;
      }
      case DataType::UINT8:
      case DataType::UINT16:
      case DataType::UINT32: {
        auto value = tensor.uint_contents().data();
        std::memcpy(dest, value, size * sizeof(uint32_t));
        input.setDatatype(DataType::UINT32);
        break;
      }
      case DataType::UINT64: {
        std::memcpy(dest, tensor.uint64_contents().data(),
                    size * sizeof(uint64_t));
        break;
      }
      case DataType::INT8:
      case DataType::INT16:
      case DataType::INT32: {
        std::memcpy(dest, tensor.int_contents().data(), size * sizeof(int32_t));
        input.setDatatype(DataType::INT32);
        break;
      }
      case DataType::INT64: {
        std::memcpy(dest, tensor.int64_contents().data(),
                    size * sizeof(int64_t));
        break;
      }
      case DataType::FP16: {
        // FIXME(varunsh): this is not handled
        std::cout << "Writing FP16 not supported\n";
        break;
      }
      case DataType::FP32: {
        std::memcpy(dest, tensor.fp32_contents().data(), size * sizeof(float));
        break;
      }
      case DataType::FP64: {
        std::memcpy(dest, tensor.fp64_contents().data(), size * sizeof(double));
        break;
      }
      case DataType::STRING: {
        std::memcpy(dest, tensor.bytes_contents().data(),
                    size * sizeof(std::byte));
        break;
      }
      default:
        // TODO(varunsh): what should we do here?
        std::cout << "Unknown datatype\n";
        break;
    }

    input.data_ = dest;
    return input;
  }
};

using InputBuilder =
  InferenceRequestInputBuilder<inference::ModelInferRequest_InferInputTensor>;

InferenceRequestPtr RequestBuilder::build(
  const CallDataModelInfer *req, size_t &buffer_index,
  const std::vector<BufferRawPtrs> &input_buffers,
  std::vector<size_t> &input_offsets,
  const std::vector<BufferRawPtrs> &output_buffers,
  std::vector<size_t> &output_offsets, const size_t &batch_size,
  size_t &batch_offset) {
  auto request = std::make_shared<InferenceRequest>();
  auto &grpc_request = req->getRequest();

  request->id_ = grpc_request.id();

  request->parameters_ = addParameters(grpc_request.parameters());

  request->callback_ = nullptr;

  auto buffer_index_backup = buffer_index;
  auto batch_offset_backup = batch_offset;

  for (const auto &input : grpc_request.inputs()) {
    try {
      auto buffers = input_buffers[buffer_index];
      for (size_t i = 0; i < buffers.size(); i++) {
        auto &buffer = buffers[i];
        auto &offset = input_offsets[buffer_index];

        request->inputs_.push_back(
          std::move(InputBuilder::build(input, buffer, offset)));
        offset += request->inputs_.back().getSize();
      }
    } catch (const std::invalid_argument &e) {
      throw;
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

  if (grpc_request.outputs_size() != 0) {
    for (auto &output : grpc_request.outputs()) {
      // TODO(varunsh): we're ignoring incoming output data
      (void)output;
      try {
        auto buffers = output_buffers[buffer_index];
        for (size_t i = 0; i < buffers.size(); i++) {
          auto &buffer = buffers[i];
          auto &offset = output_offsets[buffer_index];

          request->outputs_.emplace_back();
          request->outputs_.back().setData(
            static_cast<std::byte *>(buffer->data()) + offset);
        }
      } catch (const std::invalid_argument &e) {
        throw;
      }
      batch_offset++;
      if (batch_offset == batch_size) {
        batch_offset = 0;
        buffer_index++;
        std::fill(output_offsets.begin(), output_offsets.end(), 0);
      }
    }
  } else {
    for (const auto &input : grpc_request.inputs()) {
      (void)input;  // suppress unused variable warning
      try {
        auto buffers = output_buffers[buffer_index];
        for (size_t j = 0; j < buffers.size(); j++) {
          auto &buffer = buffers[j];
          const auto &offset = output_offsets[buffer_index];

          request->outputs_.emplace_back();
          request->outputs_.back().setData(
            static_cast<std::byte *>(buffer->data()) + offset);
        }
      } catch (const std::invalid_argument &e) {
        throw;
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

}  // namespace proteus
