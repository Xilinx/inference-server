// Copyright 2022 Xilinx, Inc.
// Copyright 2022 Advanced Micro Devices, Inc.
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

#include "amdinfer/clients/http_internal.hpp"

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
#include <cstring>      // for memcpy
#include <string_view>  // for basic_string_view
#include <utility>      // for move
#include <variant>      // for visit

#include "amdinfer/buffers/buffer.hpp"             // for Buffer
#include "amdinfer/core/data_types.hpp"            // for DataType, mapTypeToStr
#include "amdinfer/core/exceptions.hpp"            // invalid_argument
#include "amdinfer/core/predict_api_internal.hpp"  // for InferenceRequestOutput
#include "amdinfer/core/protocol_wrapper.hpp"  // for ProtocolWrappers, Inte...
#include "amdinfer/observation/logging.hpp"    // for Logger
#include "amdinfer/util/traits.hpp"            // IWYU pragma: keep
#include "half/half.hpp"                       // for half

namespace amdinfer {

ParameterMapPtr mapJsonToParameters(Json::Value json) {
  auto parameters = std::make_shared<ParameterMap>();
  for (auto const &id : json.getMemberNames()) {
    if (json[id].isString()) {
      parameters->put(id, json[id].asString());
    } else if (json[id].isBool()) {
      parameters->put(id, json[id].asBool());
    } else if (json[id].isUInt()) {
      parameters->put(id, json[id].asInt());
    } else if (json[id].isDouble()) {
      parameters->put(id, json[id].asDouble());
    } else {
      throw invalid_argument("Unknown parameter type, skipping");
    }
  }
  return parameters;
}

// refer to cppreference for std::visit
// helper type for the visitor #4
template <class... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

Json::Value mapParametersToJson(ParameterMap *parameters) {
  Json::Value json = Json::objectValue;

  for (const auto &parameter : *parameters) {
    const auto &key = parameter.first;
    const auto &value = parameter.second;
    std::visit(Overloaded{[&](bool arg) { json[key] = arg; },
                          [&](double arg) { json[key] = arg; },
                          [&](int32_t arg) { json[key] = arg; },
                          [&](const std::string &arg) { json[key] = arg; }},
               value);
  }

  return json;
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

using drogon::HttpStatusCode;

#ifdef AMDINFER_ENABLE_TRACING
void propagate(drogon::HttpResponse *resp, const StringMap &context) {
  for (const auto &[key, value] : context) {
    resp->addHeader(key, value);
  }
}
#endif

Json::Value modelMetadataTensorToJson(const ModelMetadataTensor &metadata) {
  Json::Value ret;
  ret["name"] = metadata.getName();
  ret["datatype"] = metadata.getDataType().str();
  ret["shape"] = Json::arrayValue;
  for (const auto &index : metadata.getShape()) {
    ret["shape"].append(static_cast<Json::UInt64>(index));
  }
  return ret;
}

Json::Value modelMetadataToJson(const ModelMetadata &metadata) {
  Json::Value ret;
  ret["name"] = metadata.getName();
  ret["versions"] = Json::arrayValue;
  ret["platform"] = metadata.getPlatform();
  ret["inputs"] = Json::arrayValue;
  for (const auto &input : metadata.getInputs()) {
    ret["inputs"].append(modelMetadataTensorToJson(input));
  }
  ret["outputs"] = Json::arrayValue;
  for (const auto &output : metadata.getOutputs()) {
    ret["inputs"].append(modelMetadataTensorToJson(output));
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

}  // namespace amdinfer
