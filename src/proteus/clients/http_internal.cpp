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
 * @brief Implements the internal objects used for HTTP/REST
 */

#include "proteus/clients/http_internal.hpp"

#include <json/value.h>

#include "proteus/buffers/buffer.hpp"
#include "proteus/core/predict_api_internal.hpp"
#include "proteus/observation/logging.hpp"

namespace proteus {

using types::DataType;

RequestParametersPtr addParameters(Json::Value parameters) {
#ifdef PROTEUS_ENABLE_LOGGING
  auto logger = getLogger();
#endif
  auto parameters_ = std::make_shared<RequestParameters>();
  for (auto const &id : parameters.getMemberNames()) {
    if (parameters[id].isString()) {
      parameters_->put(id, parameters[id].asString());
    } else if (parameters[id].isBool()) {
      parameters_->put(id, parameters[id].asBool());
    } else if (parameters[id].isUInt()) {
      parameters_->put(id, static_cast<int32_t>(parameters[id].asInt()));
    } else if (parameters[id].isDouble()) {
      parameters_->put(id, parameters[id].asDouble());
    } else {
      SPDLOG_LOGGER_WARN(logger, "Unknown parameter type, skipping");
    }
  }
#ifdef PROTEUS_LOGGING_ACTIVE
  (void)logger;  // suppress unused variable warning
#endif
  return parameters_;
}

template <>
class InferenceRequestInputBuilder<std::shared_ptr<Json::Value>> {
 public:
  static InferenceRequestInput build(std::shared_ptr<Json::Value> const &req,
                                     Buffer *input_buffer, size_t offset) {
    InferenceRequestInput input;
#ifdef PROTEUS_ENABLE_LOGGING
    auto logger = getLogger();
#endif
    input.data_ = input_buffer->data();

    input.shared_data_ = nullptr;
    if (!req->isMember("name")) {
      throw std::invalid_argument("No 'name' key present in request input");
    }
    input.name_ = req->get("name", "").asString();
    if (!req->isMember("shape")) {
      throw std::invalid_argument("No 'shape' key present in request input");
    }
    auto shape = req->get("shape", Json::arrayValue);
    for (auto const &i : shape) {
      if (!i.isUInt64()) {
        throw std::invalid_argument(
          "'shape' must be specified by uint64 elements");
      }
      input.shape_.push_back(i.asUInt64());
    }
    if (!req->isMember("datatype")) {
      throw std::invalid_argument("No 'datatype' key present in request input");
    }
    std::string data_type_str = req->get("datatype", "").asString();
    input.dataType_ = types::mapStrToType(data_type_str);
    if (req->isMember("parameters")) {
      auto parameters = req->get("parameters", Json::objectValue);
      input.parameters_ = addParameters(parameters);
    } else {
      input.parameters_ = std::make_unique<RequestParameters>();
    }
    if (!req->isMember("data")) {
      throw std::invalid_argument("No 'data' key present in request input");
    }
    auto data = req->get("data", Json::arrayValue);
    try {
      for (auto const &i : data) {
        switch (input.dataType_) {
          case DataType::BOOL:
            offset = input_buffer->write(i.asBool(), offset);
            break;
          case DataType::UINT8:
            offset =
              input_buffer->write(static_cast<uint8_t>(i.asUInt()), offset);
            break;
          case DataType::UINT16:
            offset =
              input_buffer->write(static_cast<uint16_t>(i.asUInt()), offset);
            break;
          case DataType::UINT32:
            offset =
              input_buffer->write(static_cast<uint32_t>(i.asUInt()), offset);
            break;
          case DataType::UINT64:
            offset =
              input_buffer->write(static_cast<uint64_t>(i.asUInt64()), offset);
            break;
          case DataType::INT8:
            offset =
              input_buffer->write(static_cast<int8_t>(i.asInt()), offset);
            break;
          case DataType::INT16:
            offset =
              input_buffer->write(static_cast<int16_t>(i.asInt()), offset);
            break;
          case DataType::INT32:
            offset =
              input_buffer->write(static_cast<int32_t>(i.asInt()), offset);
            break;
          case DataType::INT64:
            offset =
              input_buffer->write(static_cast<int64_t>(i.asInt64()), offset);
            break;
          case DataType::FP16:
            // FIXME(varunsh): this is not handled
            SPDLOG_LOGGER_WARN(logger, "Writing FP16 not supported");
            break;
          case DataType::FP32:
            offset =
              input_buffer->write(static_cast<float>(i.asFloat()), offset);
            break;
          case DataType::FP64:
            offset =
              input_buffer->write(static_cast<double>(i.asDouble()), offset);
            break;
          case DataType::STRING:
            offset = input_buffer->write(i.asString(), offset);
            break;
          default:
            // TODO(varunsh): what should we do here?
            SPDLOG_LOGGER_WARN(logger, "Unknown datatype");
            break;
        }
      }
    } catch (const Json::LogicError &e) {
      throw std::invalid_argument(
        "Could not convert some data to the provided data type");
    }
    return input;
  }
};

using InputBuilder = InferenceRequestInputBuilder<std::shared_ptr<Json::Value>>;

template <>
class InferenceRequestOutputBuilder<std::shared_ptr<Json::Value>> {
 public:
  static InferenceRequestOutput build(const std::shared_ptr<Json::Value> &req) {
    InferenceRequestOutput output;
    output.data_ = nullptr;
    output.name_ = req->get("name", "").asString();
    if (req->isMember("parameters")) {
      auto parameters = req->get("parameters", Json::objectValue);
      output.parameters_ = addParameters(parameters);
    } else {
      output.parameters_ = std::make_unique<RequestParameters>();
    }
    return output;
  }
};

using OutputBuilder =
  InferenceRequestOutputBuilder<std::shared_ptr<Json::Value>>;

InferenceRequestPtr RequestBuilder::build(
  const std::shared_ptr<Json::Value> &req, size_t &buffer_index,
  const std::vector<BufferRawPtrs> &input_buffers,
  std::vector<size_t> &input_offsets,
  const std::vector<BufferRawPtrs> &output_buffers,
  std::vector<size_t> &output_offsets, const size_t &batch_size,
  size_t &batch_offset) {
  auto request = std::make_shared<InferenceRequest>();

  if (req->isMember("id")) {
    request->id_ = req->get("id", "").asString();
  } else {
    request->id_ = "";
  }
  if (req->isMember("parameters")) {
    auto parameters = req->get("parameters", Json::objectValue);
    request->parameters_ = addParameters(parameters);
  } else {
    request->parameters_ = std::make_unique<RequestParameters>();
  }

  if (!req->isMember("inputs")) {
    throw std::invalid_argument("No 'inputs' key present in request");
  }
  auto inputs = req->get("inputs", Json::arrayValue);
  if (!inputs.isArray()) {
    throw std::invalid_argument("'inputs' is not an array");
  }

  request->callback_ = nullptr;

  auto buffer_index_backup = buffer_index;
  auto batch_offset_backup = batch_offset;
  for (auto const &i : inputs) {
    if (!i.isObject()) {
      throw std::invalid_argument(
        "At least one element in 'inputs' is not an obj");
    }
    try {
      auto &buffers = input_buffers[buffer_index];
      for (size_t j = 0; j < buffers.size(); j++) {
        auto &buffer = buffers[j];
        auto &offset = input_offsets[buffer_index];

        auto input =
          InputBuilder::build(std::make_shared<Json::Value>(i), buffer, offset);
        offset += input.getSize();

        request->inputs_.push_back(std::move(input));
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
  if (req->isMember("outputs")) {
    auto outputs = req->get("outputs", Json::arrayValue);
    for (auto const &i : outputs) {
      try {
        auto buffers = output_buffers[buffer_index];
        for (size_t j = 0; j < buffers.size(); j++) {
          auto &buffer = buffers[j];
          auto &offset = output_offsets[buffer_index];

          auto output = OutputBuilder::build(std::make_shared<Json::Value>(i));
          output.setData(static_cast<std::byte *>(buffer->data()) + offset);
          request->outputs_.push_back(std::move(output));
          // output += request->outputs_.back().getSize(); // see TODO
        }
      } catch (const std::invalid_argument &e) {
        throw;
      }
    }
  } else {
    for (auto const &i : inputs) {
      (void)i;  // suppress unused variable warning
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

using RequestBuilder = InferenceRequestBuilder<std::shared_ptr<Json::Value>>;

Json::Value ModelMetadataTensorToJson(const ModelMetadataTensor &metadata) {
  Json::Value ret;
  ret["name"] = metadata.getName();
  ret["datatype"] = types::mapTypeToStr(metadata.getDataType());
  ret["shape"] = Json::arrayValue;
  for (const auto &index : metadata.getShape()) {
    ret["shape"].append(static_cast<Json::UInt64>(index));
  }
  return ret;
}

Json::Value ModelMetadataToJson(const ModelMetadata &metadata) {
  Json::Value ret;
  ret["name"] = metadata.getName();
  ret["versions"] = Json::arrayValue;
  ret["platform"] = metadata.getPlatform();
  ret["inputs"] = Json::arrayValue;
  for (const auto &input : metadata.getInputs()) {
    ret["inputs"].append(ModelMetadataTensorToJson(input));
  }
  ret["outputs"] = Json::arrayValue;
  for (const auto &output : metadata.getOutputs()) {
    ret["inputs"].append(ModelMetadataTensorToJson(output));
  }
  return ret;
}

}  // namespace proteus
