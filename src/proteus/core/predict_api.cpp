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
 * @brief Implements the objects used to hold incoming inference requests
 */

#include "proteus/core/predict_api.hpp"

#include <json/value.h>  // for Value, arrayValue, object...

#include <cstring>    // for memcpy
#include <numeric>    // for accumulate
#include <stdexcept>  // for invalid_argument
#include <utility>    // for pair, move, make_pair

#include "proteus/buffers/buffer.hpp"       // for Buffer
#include "proteus/build_options.hpp"        // for PROTEUS_ENABLE_LOGGING
#include "proteus/observation/logging.hpp"  // for getLogger, SPDLOG_LOGGER_...

// taken from https://stackoverflow.com/a/46711735
// used for hashing strings for switch statements
constexpr unsigned int hash(const char *s, int off = 0) {
  // NOLINTNEXTLINE
  return !s[off] ? 5381 : (hash(s, off + 1) * 33) ^ s[off];
}

namespace proteus {

using types::DataType;

void RequestParameters::put(const std::string &key, bool value) {
  this->parameters_.insert(std::make_pair(key, value));
}

void RequestParameters::put(const std::string &key, double value) {
  this->parameters_.insert(std::make_pair(key, value));
}

void RequestParameters::put(const std::string &key, int32_t value) {
  this->parameters_.insert(std::make_pair(key, value));
}

void RequestParameters::put(const std::string &key, const std::string &value) {
  this->parameters_.insert(std::make_pair(key, value));
}

void RequestParameters::put(const std::string &key, const char *value) {
  this->parameters_.insert(std::make_pair(key, std::string(value)));
}

void RequestParameters::erase(const std::string &key) {
  if (this->parameters_.find(key) != this->parameters_.end()) {
    this->parameters_.erase(key);
  }
}

bool RequestParameters::has(const std::string &key) {
  return this->parameters_.find(key) != this->parameters_.end();
}

size_t RequestParameters::size() const { return parameters_.size(); }

bool RequestParameters::empty() const { return parameters_.empty(); }

std::map<std::string, Parameter> RequestParameters::data() const {
  return parameters_;
}

template <>
Parameter *RequestParameters::get(const std::string &key) {
  if (this->parameters_.find(key) != this->parameters_.end()) {
    return &this->parameters_.at(key);
  }
  return nullptr;
}

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

InferenceRequest::InferenceRequest(InferenceRequestInput &req,
                                   size_t &buffer_index,
                                   std::vector<BufferRawPtrs> input_buffers,
                                   std::vector<size_t> &input_offsets,
                                   std::vector<BufferRawPtrs> output_buffers,
                                   std::vector<size_t> &output_offsets,
                                   const size_t &batch_size,
                                   size_t &batch_offset) {
  this->id_ = "";
  this->parameters_ = std::make_unique<RequestParameters>();
  this->callback_ = nullptr;

  try {
    auto buffers = input_buffers[buffer_index];
    for (size_t i = 0; i < buffers.size(); i++) {
      auto &buffer = buffers[i];
      auto &offset = input_offsets[i];

      this->inputs_.emplace_back(req, buffer, offset);
      offset += this->inputs_.back().getSize();
    }
  } catch (const std::invalid_argument &e) {
    throw;
  }

  try {
    auto buffers = output_buffers[buffer_index];
    for (size_t i = 0; i < buffers.size(); i++) {
      auto &buffer = buffers[i];
      const auto &offset = output_offsets[i];

      this->outputs_.emplace_back();
      this->outputs_.back().setData(static_cast<std::byte *>(buffer->data()) +
                                    offset);
      // TODO(varunsh): output_offset is currently ignored! The size of the
      // output needs to come from the worker but we have no such information.
    }
  } catch (const std::invalid_argument &e) {
    throw;
  }

  batch_offset++;
  if (batch_offset == batch_size) {
    batch_offset = 0;
    buffer_index++;
    std::fill(input_offsets.begin(), input_offsets.end(), 0);
    std::fill(output_offsets.begin(), output_offsets.end(), 0);
  }
}

InferenceRequest::InferenceRequest(std::shared_ptr<Json::Value> const &req) {
  if (req->isMember("id")) {
    this->id_ = req->get("id", "").asString();
  } else {
    this->id_ = "";
  }
  if (req->isMember("parameters")) {
    auto parameters = req->get("parameters", Json::objectValue);
    this->parameters_ = addParameters(parameters);
  } else {
    this->parameters_ = std::make_unique<RequestParameters>();
  }

  if (!req->isMember("inputs")) {
    throw std::invalid_argument("No 'inputs' key present in request");
  }
  auto inputs = req->get("inputs", Json::arrayValue);
  if (!inputs.isArray()) {
    throw std::invalid_argument("'inputs' is not an array");
  }

  this->callback_ = nullptr;
}

InferenceRequest::InferenceRequest(std::shared_ptr<Json::Value> const &req,
                                   size_t &buffer_index,
                                   std::vector<BufferRawPtrs> input_buffers,
                                   std::vector<size_t> &input_offsets,
                                   std::vector<BufferRawPtrs> output_buffers,
                                   std::vector<size_t> &output_offsets,
                                   const size_t &batch_size,
                                   size_t &batch_offset)
  : InferenceRequest(req) {
  auto inputs = req->get("inputs", Json::arrayValue);

  auto buffer_index_backup = buffer_index;
  auto batch_offset_backup = batch_offset;
  for (auto const &i : inputs) {
    if (!i.isObject()) {
      throw std::invalid_argument(
        "At least one element in 'inputs' is not an obj");
    }
    try {
      auto buffers = input_buffers[buffer_index];
      for (size_t j = 0; j < buffers.size(); j++) {
        auto &buffer = buffers[j];
        auto &offset = input_offsets[j];

        InferenceRequestInput input(std::make_shared<Json::Value>(i), buffer,
                                    offset);
        offset += input.getSize();

        this->inputs_.push_back(std::move(input));
      }
    } catch (const std::invalid_argument &e) {
      throw;
    }
    batch_offset++;
    if (batch_offset == batch_size) {
      batch_offset = 0;
      buffer_index++;
      std::fill(input_offsets.begin(), input_offsets.end(), 0);
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
          auto &offset = output_offsets[j];

          this->outputs_.emplace_back(std::make_shared<Json::Value>(i));
          this->outputs_.back().setData(
            static_cast<std::byte *>(buffer->data()) + offset);
          // output += this->outputs_.back().getSize(); // see TODO
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
          const auto &offset = output_offsets[j];

          this->outputs_.emplace_back();
          this->outputs_.back().setData(
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
}

void InferenceRequest::setCallback(Callback &&callback) {
  callback_ = callback;
}

void InferenceRequest::runCallbackOnce(const InferenceResponse &response) {
  if (this->callback_ != nullptr) {
    (this->callback_)(response);
    this->callback_ = nullptr;
  }
}

void InferenceRequest::runCallback(const InferenceResponse &response) {
  (this->callback_)(response);
}

std::vector<InferenceRequestInput> InferenceRequest::getInputs() {
  return this->inputs_;
}

size_t InferenceRequest::getInputSize() { return this->inputs_.size(); }

std::vector<InferenceRequestOutput> InferenceRequest::getOutputs() {
  return this->outputs_;
}

DataType parseDatatypeStr(std::string const &data_type_str) {
  DataType data_type = DataType::BOOL;
  switch (hash(data_type_str.c_str())) {
    case hash("BOOL"):
      data_type = DataType::BOOL;
      break;
    case hash("UINT8"):
      data_type = DataType::UINT8;
      break;
    case hash("UINT16"):
      data_type = DataType::UINT16;
      break;
    case hash("UINT32"):
      data_type = DataType::UINT32;
      break;
    case hash("UINT64"):
      data_type = DataType::UINT64;
      break;
    case hash("INT8"):
      data_type = DataType::INT8;
      break;
    case hash("INT16"):
      data_type = DataType::INT16;
      break;
    case hash("INT32"):
      data_type = DataType::INT32;
      break;
    case hash("INT64"):
      data_type = DataType::INT64;
      break;
    case hash("FP16"):
      data_type = DataType::FP16;
      break;
    case hash("FP32"):
      data_type = DataType::FP32;
      break;
    case hash("FP64"):
      data_type = DataType::FP64;
      break;
    case hash("STRING"):
      data_type = DataType::STRING;
      break;
    default:
      throw std::invalid_argument("Unknown datatype " + data_type_str);
  }
  return data_type;
}

InferenceRequestInput::InferenceRequestInput(
  std::shared_ptr<Json::Value> const &req, Buffer *input_buffer,
  size_t offset) {
#ifdef PROTEUS_ENABLE_LOGGING
  auto logger = getLogger();
#endif
  this->data_ = input_buffer->data();
  ;
  this->shared_data_ = nullptr;
  if (!req->isMember("name")) {
    throw std::invalid_argument("No 'name' key present in request input");
  }
  this->name_ = req->get("name", "").asString();
  if (!req->isMember("shape")) {
    throw std::invalid_argument("No 'shape' key present in request input");
  }
  auto shape = req->get("shape", Json::arrayValue);
  for (auto const &i : shape) {
    if (!i.isUInt64()) {
      throw std::invalid_argument(
        "'shape' must be specified by uint64 elements");
    }
    this->shape_.push_back(i.asUInt64());
  }
  if (!req->isMember("datatype")) {
    throw std::invalid_argument("No 'datatype' key present in request input");
  }
  std::string data_type_str = req->get("datatype", "").asString();
  this->dataType_ = parseDatatypeStr(data_type_str);
  if (req->isMember("parameters")) {
    auto parameters = req->get("parameters", Json::objectValue);
    this->parameters_ = addParameters(parameters);
  } else {
    this->parameters_ = std::make_unique<RequestParameters>();
  }
  if (!req->isMember("data")) {
    throw std::invalid_argument("No 'data' key present in request input");
  }
  auto data = req->get("data", Json::arrayValue);
  try {
    for (auto const &i : data) {
      switch (this->dataType_) {
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
          offset = input_buffer->write(static_cast<int8_t>(i.asInt()), offset);
          break;
        case DataType::INT16:
          offset = input_buffer->write(static_cast<int16_t>(i.asInt()), offset);
          break;
        case DataType::INT32:
          offset = input_buffer->write(static_cast<int32_t>(i.asInt()), offset);
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
          offset = input_buffer->write(static_cast<float>(i.asFloat()), offset);
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
}

InferenceRequestInput::InferenceRequestInput(InferenceRequestInput &req,
                                             Buffer *input_buffer,
                                             size_t offset) {
  this->data_ = req.data_;
  this->shared_data_ = nullptr;
  this->name_ = req.name_;
  this->shape_.reserve(req.shape_.size());
  this->shape_ = req.shape_;
  this->dataType_ = req.dataType_;
  this->parameters_ = std::move(req.parameters_);
  auto size = std::accumulate(this->shape_.begin(), this->shape_.end(), 1,
                              std::multiplies<>()) *
              types::getSize(this->dataType_);
  auto *dest = static_cast<std::byte *>(input_buffer->data()) + offset;
  memcpy(dest, req.data_, size);

  this->data_ = dest;
}

InferenceRequestInput::InferenceRequestInput(void *data,
                                             std::vector<uint64_t> shape,
                                             types::DataType dataType,
                                             std::string name) {
  this->data_ = data;
  this->shared_data_ = nullptr;
  this->shape_ = shape;
  this->dataType_ = dataType;
  this->name_ = std::move(name);
  this->parameters_ = std::make_unique<RequestParameters>();
}

InferenceRequestInput::InferenceRequestInput() {
  this->data_ = nullptr;
  this->shared_data_ = nullptr;
  this->name_ = "";
  this->dataType_ = DataType::UINT32;
  this->parameters_ = std::make_unique<RequestParameters>();
}

void InferenceRequestInput::setName(std::string name) {
  this->name_ = std::move(name);
}

void InferenceRequestInput::setDatatype(types::DataType type) {
  this->dataType_ = type;
}

size_t InferenceRequestInput::getSize() {
  return std::accumulate(this->shape_.begin(), this->shape_.end(), 1,
                         std::multiplies<>());
}

void *InferenceRequestInput::getData() const {
  if (this->shared_data_ != nullptr) {
    return this->shared_data_.get();
  }
  return this->data_;
}

InferenceRequestOutput::InferenceRequestOutput() {
  this->data_ = nullptr;
  this->name_ = "";
  this->parameters_ = std::make_unique<RequestParameters>();
}

InferenceRequestOutput::InferenceRequestOutput(
  std::shared_ptr<Json::Value> const &req) {
  this->data_ = nullptr;
  this->name_ = req->get("name", "").asString();
  if (req->isMember("parameters")) {
    auto parameters = req->get("parameters", Json::objectValue);
    this->parameters_ = addParameters(parameters);
  } else {
    this->parameters_ = std::make_unique<RequestParameters>();
  }
}

void InferenceRequestOutput::setName(const std::string &name) {
  this->name_ = name;
}

InferenceResponse::InferenceResponse() {
  this->id_ = "";
  this->parameters_ = std::make_unique<RequestParameters>();
}

void InferenceResponse::setID(const std::string &id) { this->id_ = id; }

void InferenceResponse::setModel(const std::string &model) {
  this->model_ = model;
}

std::string InferenceResponse::getModel() { return this->model_; }

void InferenceResponse::addOutput(const InferenceResponseOutput &output) {
  this->outputs_.push_back(output);
}

std::vector<InferenceResponseOutput> InferenceResponse::getOutputs() const {
  return this->outputs_;
}

ModelMetadataTensor::ModelMetadataTensor(const std::string &name,
                                         types::DataType datatype,
                                         std::vector<uint64_t> shape) {
  this->name_ = name;
  this->datatype_ = datatype;
  this->shape_ = std::move(shape);
}

Json::Value ModelMetadataTensor::toJson() {
  Json::Value ret;
  ret["name"] = this->name_;
  ret["datatype"] = types::mapTypeToStr(this->datatype_);
  ret["shape"] = Json::arrayValue;
  for (auto i = 0U; i < this->shape_.size(); i++) {
    ret["shape"][i] = static_cast<Json::UInt64>(this->shape_[i]);
  }
  return ret;
}

ModelMetadata::ModelMetadata(const std::string &name,
                             const std::string &platform) {
  this->name_ = name;
  this->platform_ = platform;
  this->ready_ = false;
}

void ModelMetadata::addInputTensor(const std::string &name,
                                   types::DataType datatype,
                                   std::initializer_list<uint64_t> shape) {
  this->inputs_.emplace_back(name, datatype, shape);
}

void ModelMetadata::addInputTensor(const std::string &name,
                                   types::DataType datatype,
                                   std::vector<int> shape) {
  std::vector<uint64_t> new_shape;
  std::copy(shape.begin(), shape.end(), std::back_inserter(new_shape));
  this->inputs_.emplace_back(name, datatype, new_shape);
}

void ModelMetadata::addOutputTensor(const std::string &name,
                                    types::DataType datatype,
                                    std::initializer_list<uint64_t> shape) {
  this->outputs_.emplace_back(name, datatype, shape);
}

void ModelMetadata::addOutputTensor(const std::string &name,
                                    types::DataType datatype,
                                    std::vector<int> shape) {
  std::vector<uint64_t> new_shape;
  std::copy(shape.begin(), shape.end(), std::back_inserter(new_shape));
  this->outputs_.emplace_back(name, datatype, new_shape);
}

void ModelMetadata::setName(const std::string &name) { this->name_ = name; }

void ModelMetadata::setReady() { this->ready_ = true; }

void ModelMetadata::setNotReady() { this->ready_ = false; }

bool ModelMetadata::isReady() const { return this->ready_; }

Json::Value ModelMetadata::toJson() {
  Json::Value ret;
  ret["name"] = this->name_;
  ret["versions"] = Json::arrayValue;
  ret["platform"] = this->platform_;
  ret["inputs"] = Json::arrayValue;
  for (auto i = 0U; i < this->inputs_.size(); i++) {
    ret["inputs"][i] = this->inputs_[i].toJson();
  }
  ret["outputs"] = Json::arrayValue;
  for (auto i = 0U; i < this->outputs_.size(); i++) {
    ret["outputs"][i] = this->outputs_[i].toJson();
  }
  return ret;
}

}  // namespace proteus
