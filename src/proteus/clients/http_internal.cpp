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
#include "proteus/core/exceptions.hpp"            // invalid_argument
#include "proteus/core/interface.hpp"             // for InterfaceType, Inte...
#include "proteus/core/predict_api_internal.hpp"  // for InferenceRequestOutput
#include "proteus/observation/logging.hpp"        // for Logger
#include "proteus/util/compression.hpp"           // for z_decompress
#include "proteus/util/traits.hpp"                // for is_any

namespace proteus {

RequestParametersPtr mapJsonToParameters(Json::Value parameters) {
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
      throw invalid_argument("Unknown parameter type, skipping");
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

template <typename T>
constexpr auto jsonValueToType(const Json::Value &datum) {
  if constexpr (std::is_same_v<T, bool>) {
    return datum.asBool();
  } else if constexpr (util::is_any_v<T, uint8_t, uint16_t, uint32_t>) {
    return datum.asUInt();
  } else if constexpr (std::is_same_v<T, uint64_t>) {
    return datum.asUInt64();
  } else if constexpr (util::is_any_v<T, int8_t, int16_t, int32_t>) {
    return datum.asInt();
  } else if constexpr (std::is_same_v<T, int64_t>) {
    return datum.asInt64();
  } else if constexpr (util::is_any_v<T, fp16, float>) {
    return datum.asFloat();
  } else if constexpr (std::is_same_v<T, double>) {
    return datum.asDouble();
  } else if constexpr (std::is_same_v<T, char>) {
    return datum.asString();
  } else {
    static_assert(!sizeof(T), "Invalid type to SetOutputData");
  }
}

struct SetOutputData {
  template <typename T>
  void operator()(const Json::Value &json,
                  InferenceResponseOutput *output) const {
    if constexpr (std::is_same_v<T, char>) {
      assert(json.size() == 1);
      auto str = json[0].asString();
      std::vector<std::byte> data;
      data.resize(str.length());
      memcpy(data.data(), str.data(), str.length());
      output->setData(std::move(data));
    } else {
      std::vector<std::byte> data;
      data.resize(json.size() * sizeof(T));
      auto *ptr = data.data();
      for (const auto &datum : json) {
        auto raw_datum = static_cast<T>(jsonValueToType<T>(datum));
        memcpy(ptr, &raw_datum, sizeof(T));
        ptr += sizeof(T);
      }
      output->setData(std::move(data));
    }
  }
};

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
    switchOverTypes(SetOutputData(), output.getDatatype(), json_data, &output);
    response.addOutput(output);
  }

  return response;
}

struct SetInputData {
  template <typename T>
  void operator()(Json::Value *json, void *src_data, size_t src_size) const {
    auto *data = static_cast<T *>(src_data);
    if constexpr (std::is_same_v<T, char>) {
      std::string str{data, src_size};
      json->append(str);
    } else {
      constexpr auto getData = [](const T *data_ptr, size_t index) {
        if constexpr (std::is_same_v<T, uint64_t>) {
          return static_cast<Json::UInt64>(data_ptr[index]);
        } else if constexpr (std::is_same_v<T, int64_t>) {
          return static_cast<Json::Int64>(data_ptr[index]);
        } else if constexpr (util::is_any_v<T, bool, uint8_t, uint16_t,
                                            uint32_t, int8_t, int16_t, int32_t,
                                            float, double>) {
          return data_ptr[index];
        } else if constexpr (util::is_any_v<T, fp16>) {
          return half_float::half_cast<float>(data_ptr[index]);
        } else {
          static_assert(!sizeof(T), "Invalid type to SetInputData");
        }
      };

      for (auto i = 0U; i < src_size; ++i) {
        json->append(getData(data, i));
      }
    }
  }
};

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
    switchOverTypes(SetInputData(), input.getDatatype(), &(json_input["data"]),
                    input.getData(), input.getSize());
    json["inputs"].append(json_input);
  }

  // TODO(varunsh): omitting outputs for now

  return json;
}

struct WriteData {
  template <typename T>
  size_t operator()(Buffer *buffer, const Json::Value &value,
                    size_t offset) const {
    if constexpr (std::is_same_v<T, char>) {
      return buffer->write(jsonValueToType<T>(value), offset);
    } else {
      return buffer->write(static_cast<T>(jsonValueToType<T>(value)), offset);
    }
  }
};

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

    if (!req->isMember("name")) {
      throw invalid_argument("No 'name' key present in request input");
    }
    input.name_ = req->get("name", "").asString();
    if (!req->isMember("shape")) {
      throw invalid_argument("No 'shape' key present in request input");
    }
    auto shape = req->get("shape", Json::arrayValue);
    for (auto const &i : shape) {
      if (!i.isUInt64()) {
        throw invalid_argument("'shape' must be specified by uint64 elements");
      }
      input.shape_.push_back(i.asUInt64());
    }
    if (!req->isMember("datatype")) {
      throw invalid_argument("No 'datatype' key present in request input");
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
      throw invalid_argument("No 'data' key present in request input");
    }
    auto data = req->get("data", Json::arrayValue);
    try {
      for (auto const &i : data) {
        offset = switchOverTypes(WriteData(), input.dataType_, input_buffer, i,
                                 offset);
      }
    } catch (const Json::LogicError &) {
      throw invalid_argument(
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
  const std::shared_ptr<Json::Value> &req, const BufferRawPtrs &input_buffers,
  std::vector<size_t> &input_offsets, const BufferRawPtrs &output_buffers,
  std::vector<size_t> &output_offsets) {
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
    throw invalid_argument("No 'inputs' key present in request");
  }
  auto inputs = req->get("inputs", Json::arrayValue);
  if (!inputs.isArray()) {
    throw invalid_argument("'inputs' is not an array");
  }

  request->callback_ = nullptr;

  assert(input_buffers.size() == inputs.size());
  const auto input_num = input_buffers.size();
  for (auto i = 0U; i < input_num; ++i) {
    const auto &json_input = inputs[i];
    if (!json_input.isObject()) {
      throw invalid_argument("At least one element in 'inputs' is not an obj");
    }
    auto *buffer = input_buffers[i];
    auto &offset = input_offsets[i];
    auto input = InputBuilder::build(std::make_shared<Json::Value>(json_input),
                                     buffer, offset);
    offset += (input.getSize() * input.getDatatype().size());

    request->inputs_.push_back(std::move(input));
  }

  // TODO(varunsh): output_offset is currently ignored! The size of the output
  // needs to come from the worker but we have no such information.
  const auto output_num = output_buffers.size();
  if (req->isMember("outputs")) {
    auto outputs = req->get("outputs", Json::arrayValue);
    assert(output_buffers.size() == outputs.size());
    for (auto i = 0U; i < output_num; ++i) {
      const auto &json_output = outputs[i];
      auto *buffer = output_buffers[i];
      auto &offset = output_offsets[i];

      auto output =
        OutputBuilder::build(std::make_shared<Json::Value>(json_output));
      output.setData(static_cast<std::byte *>(buffer->data()) + offset);
      request->outputs_.push_back(std::move(output));
      // output += request->outputs_.back().getSize(); // see TODO
    }
  } else {
    for (auto i = 0U; i < output_num; ++i) {
      auto *buffer = output_buffers[i];
      const auto &offset = output_offsets[i];

      request->outputs_.emplace_back();
      request->outputs_.back().setData(
        static_cast<std::byte *>(buffer->data()) + offset);
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
  auto body_decompress = util::z_decompress(body.data(), body.length());
  success = reader->parse(body_decompress.data(),
                          body_decompress.data() + body_decompress.size(),
                          root.get(), &errors);
  if (success) {
    return root;
  }

  throw invalid_argument("Failed to interpret request body as JSON");
}

DrogonHttp::DrogonHttp(const drogon::HttpRequestPtr &req,
                       DrogonCallback callback)
  : req_(req), callback_(std::move(callback)) {
  this->type_ = InterfaceType::kDrogonHttp;
  this->json_ = parseJson(req.get());
}

size_t DrogonHttp::getInputSize() {
  if (!this->json_->isMember("inputs")) {
    throw invalid_argument("No 'inputs' key present in request");
  }
  auto inputs = this->json_->get("inputs", Json::arrayValue);
  if (!inputs.isArray()) {
    throw invalid_argument("'inputs' is not an arrafy");
  }

  return inputs.size();
}

Json::Value parseResponse(InferenceResponse response) {
  Json::Value ret;
  ret["model_name"] = response.getModel();
  ret["outputs"] = Json::arrayValue;
  ret["id"] = response.getID();
  auto outputs = response.getOutputs();
  for (const InferenceResponseOutput &output : outputs) {
    Json::Value json_output;
    json_output["name"] = output.getName();
    json_output["parameters"] = Json::objectValue;
    json_output["data"] = Json::arrayValue;
    json_output["shape"] = Json::arrayValue;
    json_output["datatype"] = output.getDatatype().str();
    const auto &shape = output.getShape();
    for (const size_t &index : shape) {
      json_output["shape"].append(static_cast<Json::UInt>(index));
    }

    switchOverTypes(SetInputData(), output.getDatatype(),
                    &(json_output["data"]), output.getData(), output.getSize());
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
  const BufferRawPtrs &input_buffers, std::vector<size_t> &input_offsets,
  const BufferRawPtrs &output_buffers, std::vector<size_t> &output_offsets) {
#ifdef PROTEUS_ENABLE_LOGGING
  const auto &logger = this->getLogger();
#endif
  try {
    auto request =
      RequestBuilder::build(this->json_, input_buffers, input_offsets,
                            output_buffers, output_offsets);
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
        } catch (const invalid_argument &e) {
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
  } catch (const invalid_argument &e) {
    PROTEUS_LOG_INFO(logger, e.what());
    this->callback_(
      errorHttpResponse(e.what(), HttpStatusCode::k400BadRequest));
    return nullptr;
  }
}

void DrogonHttp::errorHandler(const std::exception &e) {
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

ModelMetadata mapJsonToModelMetadata(const Json::Value *json) {
  ModelMetadata metadata{json->get("name", "").asString(),
                         json->get("platform", "").asString()};
  for (const auto &input : json->get("inputs", Json::arrayValue)) {
    std::vector<int> shape;
    shape.reserve(input["shape"].size());
    for (const auto &index : input["shape"]) {
      shape.push_back(index.asInt());
    }
    metadata.addInputTensor(input["name"].asString(),
                            DataType(input["datatype"].asString().c_str()),
                            shape);
  }
  for (const auto &output : json->get("outputs", Json::arrayValue)) {
    std::vector<int> shape;
    shape.reserve(output["shape"].size());
    for (const auto &index : output["shape"]) {
      shape.push_back(index.asInt());
    }
    metadata.addOutputTensor(output["name"].asString(),
                             DataType(output["datatype"].asString().c_str()),
                             shape);
  }
  return metadata;
}

}  // namespace proteus
