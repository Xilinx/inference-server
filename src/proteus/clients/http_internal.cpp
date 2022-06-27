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

#include <drogon/HttpRequest.h>   // for HttpRequestPtr, Htt...
#include <drogon/HttpResponse.h>  // for HttpResponsePtr
#include <drogon/HttpTypes.h>     // for HttpStatusCode, k40...
#include <json/config.h>          // for UInt64, Int64, UInt
#include <json/reader.h>          // for CharReaderBuilder
#include <json/value.h>           // for Value, arrayValue

#include <algorithm>    // for fill
#include <cassert>      // for assert
#include <cstddef>      // for size_t, byte
#include <cstdint>      // for uint64_t, int32_t
#include <iostream>     // for operator<<, cout
#include <string_view>  // for basic_string_view
#include <utility>      // for move
#include <variant>      // for visit

#include "proteus/buffers/buffer.hpp"             // for Buffer
#include "proteus/core/data_types.hpp"            // for DataType, mapTypeToStr
#include "proteus/core/interface.hpp"             // for InterfaceType, Inte...
#include "proteus/core/predict_api_internal.hpp"  // for InferenceRequestOutput
#include "proteus/helpers/compression.hpp"        // for z_decompress
#include "proteus/observation/logging.hpp"        // for Logger

namespace proteus {

RequestParametersPtr mapJsonToParameters(Json::Value parameters) {
  auto parameters_ = std::make_shared<RequestParameters>();
#ifdef PROTEUS_ENABLE_LOGGING
  Logger logger{Loggers::kClient};
#endif
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
      PROTEUS_LOG_WARN(logger, "Unknown parameter type, skipping");
    }
  }
  return parameters_;
}

// refer to cppreference for std::visit
// helper type for the visitor #4
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

Json::Value mapParametersToJson(RequestParameters *parameters) {
  Json::Value json = Json::objectValue;

  for (const auto &parameter : *parameters) {
    const auto &key = parameter.first;
    const auto &value = parameter.second;
    std::visit(overloaded{[&](bool arg) { json[key] = arg; },
                          [&](double arg) { json[key] = arg; },
                          [&](int32_t arg) { json[key] = arg; },
                          [&](const std::string &arg) { json[key] = arg; }},
               value);
  }

  return json;
}

template <typename T, typename Fn>
void setOutputData(const Json::Value &json, InferenceResponseOutput *output,
                   Fn f) {
  auto data = std::make_shared<std::vector<T>>();
  data->reserve(json.size());
  for (const auto &datum : json) {
    data->push_back((datum.*f)());
  }
  auto data_cast = std::reinterpret_pointer_cast<std::byte>(data);
  output->setData(std::move(data_cast));
}

InferenceResponse mapJsonToResponse(Json::Value *json) {
  InferenceResponse response;
  response.setModel(json->get("model_name", "").asString());
  response.setID(json->get("id", "").asString());

  auto json_outputs = json->get("outputs", Json::arrayValue);
  for (const auto &json_output : json_outputs) {
    InferenceResponseOutput output;
    output.setName(json_output["name"].asString());
    output.setParameters(mapJsonToParameters(json_output["parameters"]));
    output.setDatatype(DataType(json_output["datatype"].asCString()));

    auto json_shape = json_output["shape"];
    std::vector<uint64_t> shape;
    shape.reserve(json_shape.size());
    for (const auto &index : json_shape) {
      shape.push_back(index.asUInt());
    }
    output.setShape(shape);
    const auto &json_data = json_output["data"];
    switch (output.getDatatype()) {
      case DataType::BOOL: {
        setOutputData<bool>(json_data, &output, &Json::Value::asBool);
        break;
      }
      case DataType::UINT8: {
        setOutputData<uint8_t>(json_data, &output, &Json::Value::asUInt);
        break;
      }
      case DataType::UINT16: {
        setOutputData<uint16_t>(json_data, &output, &Json::Value::asUInt);
        break;
      }
      case DataType::UINT32: {
        setOutputData<uint32_t>(json_data, &output, &Json::Value::asUInt);
        break;
      }
      case DataType::UINT64: {
        setOutputData<uint64_t>(json_data, &output, &Json::Value::asUInt64);
        break;
      }
      case DataType::INT8: {
        setOutputData<int8_t>(json_data, &output, &Json::Value::asInt);
        break;
      }
      case DataType::INT16: {
        setOutputData<int16_t>(json_data, &output, &Json::Value::asInt);
        break;
      }
      case DataType::INT32: {
        setOutputData<int32_t>(json_data, &output, &Json::Value::asInt);
        break;
      }
      case DataType::INT64: {
        setOutputData<int64_t>(json_data, &output, &Json::Value::asInt64);
        break;
      }
      case DataType::FP16: {
        // FIXME(varunsh): this is not handled
        throw std::invalid_argument("Writing FP16 not supported at this time");
      }
      case DataType::FP32: {
        setOutputData<float>(json_data, &output, &Json::Value::asFloat);
        break;
      }
      case DataType::FP64: {
        setOutputData<double>(json_data, &output, &Json::Value::asDouble);
        break;
      }
      case DataType::STRING: {
        auto data = std::make_shared<std::string>();
        assert(json_data.size() == 1);
        auto str = json_data[0].asString();
        data->reserve(str.size());
        data->assign(str);
        auto data_cast = std::reinterpret_pointer_cast<std::byte>(data);
        output.setData(std::move(data_cast));
        break;
      }
      default:
        // TODO(varunsh): what should we do here?
        std::cout << "Unknown datatype\n";
        break;
    }
    response.addOutput(output);
  }

  return response;
}

template <typename T, typename C = T>
void setInputData(Json::Value &json, const InferenceRequestInput *input) {
  auto *data = static_cast<T *>(input->getData());
  for (size_t i = 0; i < input->getSize(); i++) {
    json.append(static_cast<C>(data[i]));
  }
}

Json::Value mapRequestToJson(const InferenceRequest &request) {
  Json::Value json;
  json["id"] = request.getID();
  auto *parameters = request.getParameters();
  json["parameters"] =
    parameters != nullptr ? mapParametersToJson(parameters) : Json::objectValue;
  json["inputs"] = Json::arrayValue;
  const auto &inputs = request.getInputs();
  for (const auto &input : inputs) {
    Json::Value json_input;
    json_input["name"] = input.getName();
    json_input["datatype"] = input.getDatatype().str();
    json_input["shape"] = Json::arrayValue;
    parameters = request.getParameters();
    json_input["parameters"] = parameters != nullptr
                                 ? mapParametersToJson(parameters)
                                 : Json::objectValue;
    for (const auto &index : input.getShape()) {
      json_input["shape"].append(static_cast<Json::UInt64>(index));
    }
    json_input["data"] = Json::arrayValue;
    switch (input.getDatatype()) {
      case DataType::BOOL: {
        setInputData<bool>(json_input["data"], &input);
        break;
      }
      case DataType::UINT8: {
        setInputData<uint8_t>(json_input["data"], &input);
        break;
      }
      case DataType::UINT16: {
        setInputData<uint16_t>(json_input["data"], &input);
        break;
      }
      case DataType::UINT32: {
        setInputData<uint32_t>(json_input["data"], &input);
        break;
      }
      case DataType::UINT64: {
        setInputData<uint64_t, Json::UInt64>(json_input["data"], &input);
        break;
      }
      case DataType::INT8: {
        setInputData<int8_t>(json_input["data"], &input);
        break;
      }
      case DataType::INT16: {
        setInputData<int16_t>(json_input["data"], &input);
        break;
      }
      case DataType::INT32: {
        setInputData<int32_t>(json_input["data"], &input);
        break;
      }
      case DataType::INT64: {
        setInputData<int64_t, Json::Int64>(json_input["data"], &input);
        break;
      }
      case DataType::FP16: {
        // FIXME(varunsh): this is not handled
        std::cout << "Writing FP16 not supported\n";
        break;
      }
      case DataType::FP32: {
        setInputData<float>(json_input["data"], &input);
        break;
      }
      case DataType::FP64: {
        setInputData<double>(json_input["data"], &input);
        break;
      }
      case DataType::STRING: {
        auto *data = static_cast<std::string *>(input.getData());
        json_input["data"].append(*data);
        break;
      }
      default:
        // TODO(varunsh): what should we do here?
        std::cout << "Unknown datatype\n";
        break;
    }
    json["inputs"].append(json_input);
  }

  // TODO(varunsh): omitting outputs for now

  return json;
}

template <>
class InferenceRequestInputBuilder<std::shared_ptr<Json::Value>> {
 public:
  static InferenceRequestInput build(std::shared_ptr<Json::Value> const &req,
                                     Buffer *input_buffer, size_t offset) {
    InferenceRequestInput input;
#ifdef PROTEUS_ENABLE_LOGGING
    Logger logger{Loggers::kServer};
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
    input.dataType_ = DataType(data_type_str.c_str());
    if (req->isMember("parameters")) {
      auto parameters = req->get("parameters", Json::objectValue);
      input.parameters_ = mapJsonToParameters(parameters);
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
            PROTEUS_LOG_WARN(logger, "Writing FP16 not supported");
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
            PROTEUS_LOG_WARN(logger, "Unknown datatype");
            break;
        }
      }
    } catch (const Json::LogicError &) {
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
      output.parameters_ = mapJsonToParameters(parameters);
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
    request->parameters_ = mapJsonToParameters(parameters);
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
    const auto &buffers = input_buffers[buffer_index];
    for (const auto &buffer : buffers) {
      auto &offset = input_offsets[buffer_index];

      auto input =
        InputBuilder::build(std::make_shared<Json::Value>(i), buffer, offset);
      offset += (input.getSize() * input.getDatatype().size());

      request->inputs_.push_back(std::move(input));
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
      auto buffers = output_buffers[buffer_index];
      for (auto &buffer : buffers) {
        auto &offset = output_offsets[buffer_index];

        auto output = OutputBuilder::build(std::make_shared<Json::Value>(i));
        output.setData(static_cast<std::byte *>(buffer->data()) + offset);
        request->outputs_.push_back(std::move(output));
        // output += request->outputs_.back().getSize(); // see TODO
      }
    }
  } else {
    for (auto const &i : inputs) {
      (void)i;  // suppress unused variable warning
      const auto &buffers = output_buffers[buffer_index];
      for (const auto &buffer : buffers) {
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

using RequestBuilder = InferenceRequestBuilder<std::shared_ptr<Json::Value>>;

using drogon::HttpStatusCode;

drogon::HttpResponsePtr errorHttpResponse(const std::string &error,
                                          int status_code) {
  Json::Value ret;
  ret["error"] = error.data();
  auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
  resp->setStatusCode(static_cast<drogon::HttpStatusCode>(status_code));
  return resp;
}

std::shared_ptr<Json::Value> parseJson(const drogon::HttpRequest *req) {
#ifdef PROTEUS_ENABLE_LOGGING
  Logger logger{Loggers::kServer};
#endif

  // attempt to get the JSON object directly first
  const auto &json_obj = req->getJsonObject();
  if (json_obj != nullptr) {
    return json_obj;
  }

  PROTEUS_LOG_DEBUG(logger, "Failed to get JSON data directly");

  // if it's not valid, then we need to attempt to parse the body
  auto root = std::make_shared<Json::Value>();

  std::string errors;
  Json::CharReaderBuilder builder;
  Json::CharReader *reader = builder.newCharReader();
  auto body = req->getBody();
  bool success =
    reader->parse(body.data(), body.data() + body.size(), root.get(), &errors);
  if (success) {
    return root;
  }

  PROTEUS_LOG_DEBUG(logger, "Failed to interpret body as JSON data");

  // if it's still not valid, attempt to uncompress the body and convert to JSON
  auto body_decompress = z_decompress(body.data(), body.length());
  success = reader->parse(body_decompress.data(),
                          body_decompress.data() + body_decompress.size(),
                          root.get(), &errors);
  if (success) {
    return root;
  }

  throw std::invalid_argument("Failed to interpret request body as JSON");
}

DrogonHttp::DrogonHttp(const drogon::HttpRequestPtr &req,
                       DrogonCallback callback)
  : req_(req), callback_(std::move(callback)) {
  this->type_ = InterfaceType::kDrogonHttp;
  this->json_ = parseJson(req.get());
}

size_t DrogonHttp::getInputSize() {
  if (!this->json_->isMember("inputs")) {
    throw std::invalid_argument("No 'inputs' key present in request");
  }
  auto inputs = this->json_->get("inputs", Json::arrayValue);
  if (!inputs.isArray()) {
    throw std::invalid_argument("'inputs' is not an array");
  }
  return inputs.size();
}

Json::Value parseResponse(InferenceResponse response) {
  Json::Value ret;
  ret["model_name"] = response.getModel();
  ret["outputs"] = Json::arrayValue;
  ret["id"] = response.getID();
  auto outputs = response.getOutputs();
  for (InferenceResponseOutput &output : outputs) {
    Json::Value json_output;
    json_output["name"] = output.getName();
    json_output["parameters"] = Json::objectValue;
    json_output["data"] = Json::arrayValue;
    json_output["shape"] = Json::arrayValue;
    json_output["datatype"] = output.getDatatype().str();
    auto shape = output.getShape();
    for (const size_t &index : shape) {
      json_output["shape"].append(static_cast<Json::UInt>(index));
    }

    switch (output.getDatatype()) {
      case DataType::BOOL: {
        auto *data = static_cast<std::vector<bool> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append(static_cast<bool>((*data)[i]));
        }
        break;
      }
      case DataType::UINT8: {
        auto *data = static_cast<std::vector<uint8_t> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append((*data)[i]);
        }
        break;
      }
      case DataType::UINT16: {
        auto *data = static_cast<std::vector<uint16_t> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append((*data)[i]);
        }
        break;
      }
      case DataType::UINT32: {
        auto *data = static_cast<std::vector<uint32_t> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append((*data)[i]);
        }
        break;
      }
      case DataType::UINT64: {
        auto *data = static_cast<std::vector<uint64_t> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append(static_cast<Json::UInt64>((*data)[i]));
        }
        break;
      }
      case DataType::INT8: {
        auto *data = static_cast<std::vector<int8_t> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append((*data)[i]);
        }
        break;
      }
      case DataType::INT16: {
        auto *data = static_cast<std::vector<int16_t> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append((*data)[i]);
        }
        break;
      }
      case DataType::INT32: {
        auto *data = static_cast<std::vector<int32_t> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append((*data)[i]);
        }
        break;
      }
      case DataType::INT64: {
        auto *data = static_cast<std::vector<int64_t> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append(static_cast<Json::Int64>((*data)[i]));
        }
        break;
      }
      case DataType::FP16: {
        // FIXME(varunsh): this is not handled
        std::cout << "Writing FP16 not supported\n";
        break;
      }
      case DataType::FP32: {
        auto *data = static_cast<std::vector<float> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append((*data)[i]);
        }
        break;
      }
      case DataType::FP64: {
        auto *data = static_cast<std::vector<double> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append((*data)[i]);
        }
        break;
      }
      case DataType::STRING: {
        auto *data = static_cast<std::string *>(output.getData());
        json_output["data"].append(*data);
        // for(size_t i = 0; i < output.getSize(); i++){
        //   json_output["data"].append(data->data()[i]);
        // }
        break;
      }
      default:
        // TODO(varunsh): what should we do here?
        std::cout << "Unknown datatype\n";
        break;
    }
    ret["outputs"].append(json_output);
  }
  return ret;
}

#ifdef PROTEUS_ENABLE_TRACING
void propagate(drogon::HttpResponse *resp, const StringMap &context) {
  for (const auto &[key, value] : context) {
    resp->addHeader(key, value);
  }
}
#endif

std::shared_ptr<InferenceRequest> DrogonHttp::getRequest(
  size_t &buffer_index, const std::vector<BufferRawPtrs> &input_buffers,
  std::vector<size_t> &input_offsets,
  const std::vector<BufferRawPtrs> &output_buffers,
  std::vector<size_t> &output_offsets, const size_t &batch_size,
  size_t &batch_offset) {
#ifdef PROTEUS_ENABLE_LOGGING
  const auto &logger = this->getLogger();
#endif
  try {
    auto request = RequestBuilder::build(
      this->json_, buffer_index, input_buffers, input_offsets, output_buffers,
      output_offsets, batch_size, batch_offset);
    Callback callback = [callback = std::move(this->callback_)](
                          const InferenceResponse &response) {
      drogon::HttpResponsePtr resp;
      if (response.isError()) {
        resp = errorHttpResponse(response.getError(),
                                 HttpStatusCode::k400BadRequest);
      } else {
        try {
          Json::Value ret = parseResponse(response);
          resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        } catch (const std::invalid_argument &e) {
          resp = errorHttpResponse(e.what(), HttpStatusCode::k400BadRequest);
        }
      }
#ifdef PROTEUS_ENABLE_TRACING
      const auto &context = response.getContext();
      propagate(resp.get(), context);
#endif
      callback(resp);
    };
    request->setCallback(std::move(callback));
    return request;
  } catch (const std::invalid_argument &e) {
    PROTEUS_LOG_INFO(logger, e.what());
    this->callback_(
      errorHttpResponse(e.what(), HttpStatusCode::k400BadRequest));
    return nullptr;
  }
}

void DrogonHttp::errorHandler(const std::invalid_argument &e) {
#ifdef PROTEUS_ENABLE_LOGGING
  const auto &logger = this->getLogger();
  PROTEUS_LOG_DEBUG(logger, e.what());
#endif
  this->callback_(errorHttpResponse(e.what(), HttpStatusCode::k400BadRequest));
}

Json::Value ModelMetadataTensorToJson(const ModelMetadataTensor &metadata) {
  Json::Value ret;
  ret["name"] = metadata.getName();
  ret["datatype"] = metadata.getDataType().str();
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
