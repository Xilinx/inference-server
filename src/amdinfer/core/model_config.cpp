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
 * @brief
 */

#include "amdinfer/core/model_config.hpp"

#include <toml++/toml.h>

#include <filesystem>

#include "amdinfer/core/exceptions.hpp"
#include "amdinfer/core/versioned_endpoint.hpp"  // for getVersionedEndpoint
#include "amdinfer/util/filesystem.hpp"
#include "model_config.pb.h"  // for Config, InferP...

namespace amdinfer {

std::string extractString(const toml::table& table, const std::string& key,
                          bool is_required) {
  if (!table.contains(key)) {
    if (is_required) {
      throw invalid_argument("The configuration must have a " + key);
    }
    return "";
  }

  const auto& node = table.at(key);
  if (!node.is_string()) {
    throw invalid_argument(key + " must be a string");
  }

  return node.value<std::string>().value();
}

ModelConfigTensor extractModelConfigTensor(const toml::table& table);
ModelConfigData extractConfig(const toml::table& table, bool is_ensemble);

// if we're not explicitly handling the types, use this to catch the failure at
// compile time
template <class...>
constexpr std::false_type kTemplatedFalse{};

template <typename T>
std::vector<T> extractArray(const toml::table& table, const std::string& key) {
  if (!table.contains(key)) {
    throw invalid_argument("The configuration must have a " + key);
  }

  const auto& node = table.at(key);
  if (!node.is_array()) {
    throw invalid_argument(key + " must be an array");
  }
  if (!node.is_homogeneous()) {
    throw invalid_argument(key + " must be a homogenous array");
  }
  const auto& array = *node.as_array();

  std::vector<T> vector;
  for (auto&& element : array) {
    if constexpr (std::is_integral_v<T>) {
      const auto optional_value = element.value<T>();
      if (optional_value) {
        vector.push_back(optional_value.value());
      } else {
        throw invalid_argument(
          "One or more values could not be converted to integer in " + key);
      }
    } else if constexpr (std::is_same_v<T, ModelConfigTensor>) {
      if (!element.is_table()) {
        throw invalid_argument(
          "One or more values could not be converted to a table");
      }
      const auto& tbl = *element.as_table();
      vector.push_back(extractModelConfigTensor(tbl));
    } else if constexpr (std::is_same_v<T, ModelConfigData>) {
      if (!element.is_table()) {
        throw invalid_argument(
          "One or more values could not be converted to a table");
      }
      const auto& tbl = *element.as_table();
      vector.push_back(extractConfig(tbl, true));
    } else {
      static_assert(kTemplatedFalse<T>);
    }
  }

  return vector;
}

ModelConfigTensor extractModelConfigTensor(const toml::table& table) {
  auto name = extractString(table, "name", true);
  auto datatype_str = extractString(table, "datatype", true);
  auto shape = extractArray<int64_t>(table, "shape");
  auto id = extractString(table, "id", false);

  DataType datatype{datatype_str.c_str()};

  return {name, shape, datatype, id};
}

ModelConfigData extractConfig(const toml::table& table, bool is_ensemble) {
  auto name = extractString(table, "name", true);
  auto platform = extractString(table, "platform", true);
  auto id = extractString(table, "id", is_ensemble);

  auto inputs = extractArray<ModelConfigTensor>(table, "inputs");
  auto outputs = extractArray<ModelConfigTensor>(table, "outputs");

  return {name, platform, id, inputs, outputs};
}

ModelConfigTensor::ModelConfigTensor(std::string name,
                                     std::vector<int64_t> shape,
                                     DataType data_type, std::string id)
  : Tensor(std::move(name), std::move(shape), data_type), id_(std::move(id)) {}

const std::string& ModelConfigTensor::id() const& { return id_; }

std::string ModelConfigTensor::id() && { return std::move(id_); }

void ModelConfig::createModels() {
  for (const auto& config : configs_) {
    models_.emplace_back(config.name, ParameterMap{});
    auto& parameters = std::get<1>(models_.back());
    std::string extension;
    if (config.platform == "tensorflow_graphdef") {
      const auto& inputs = config.inputs;
      if (inputs.size() != 1) {
        throw invalid_argument("Currently, there must be one input tensor");
      }
      const auto& input = inputs.at(0);
      parameters.put("input_node", input.getName());
      const auto& input_shape = input.getShape();
      // ZenDNN assumes square image in HWC format
      parameters.put("input_size", static_cast<int>(input_shape.at(0)));
      parameters.put("image_channels",
                     static_cast<int>(input_shape.at(input_shape.size() - 1)));

      const auto& outputs = config.outputs;
      if (outputs.size() != 1) {
        throw invalid_argument("Currently, there must be one output tensor");
      }
      const auto& output = outputs.at(0);
      parameters.put("output_node", output.getName());
      const auto& output_shape = output.getShape();
      // ZenDNN assumes [X] classes as output
      parameters.put("output_classes", static_cast<int>(output_shape.at(0)));

      parameters.put("worker", "tfzendnn");
    } else if (config.platform == "pytorch_torchscript") {
      parameters.put("worker", "ptzendnn");
    } else if (config.platform == "onnx_onnxv1" ||
               config.platform == "migraphx_mxr") {
      parameters.put("worker", "migraphx");
    } else if (config.platform == "vitis_xmodel") {
      parameters.put("worker", "xmodel");
    } else if (config.platform == "amdinfer_cpp") {
      parameters.put("worker", "cplusplus");
    } else {
      throw invalid_argument("Unknown platform: " + config.platform);
    }
  }

  // assuming a static chain for now so define it in reverse order
  for (auto i = models_.size() - 1; i-- > 0;) {
    const auto& model = std::get<0>(models_.at(i + 1));
    auto& parameters = std::get<1>(models_.at(i));
    parameters.put("next", model);
  }
}

ModelConfig::ModelConfig(const toml::table& toml, const std::string& version) {
  if (toml.empty()) {
    throw invalid_argument("The configuration cannot be empty");
  }

  if (toml.contains("models")) {
    configs_ = extractArray<ModelConfigData>(toml, "models");
  } else {
    configs_.emplace_back(extractConfig(toml, false));
  }

  for (auto& config : configs_) {
    config.name = getVersionedEndpoint(config.name, version);
  }

  this->createModels();
}

ModelConfig::ModelConfig(const inference::Config& config,
                         const std::string& version) {
  // proto does not support ensembles so id is fixed to empty
  const std::string id;

  const auto& model_name = config.name();
  const auto& platform = config.platform();

  const auto& proto_inputs = config.inputs();
  std::vector<ModelConfigTensor> inputs;
  for (const auto& input : proto_inputs) {
    const auto& name = input.name();
    const DataType datatype{input.datatype().c_str()};
    const auto& proto_shape = input.shape();
    std::vector<int64_t> shape;
    shape.reserve(proto_shape.size());
    for (const auto& index : proto_shape) {
      shape.push_back(index);
    }
    inputs.emplace_back(name, shape, datatype, id);
  }

  const auto& proto_outputs = config.outputs();
  std::vector<ModelConfigTensor> outputs;
  for (const auto& output : proto_outputs) {
    const auto& name = output.name();
    const DataType datatype{output.datatype().c_str()};
    const auto& proto_shape = output.shape();
    std::vector<int64_t> shape;
    shape.reserve(proto_shape.size());
    for (const auto& index : proto_shape) {
      shape.push_back(index);
    }
    outputs.emplace_back(name, shape, datatype, id);
  }

  configs_.emplace_back(getVersionedEndpoint(model_name, version), platform, id,
                        inputs, outputs);

  this->createModels();
}

void ModelConfig::setModelFiles(const std::filesystem::path& base_path) {
  auto i = 0;
  for (auto& [_, parameters] : models_) {
    const auto& config = configs_.at(i);
    if (config.id.empty()) {
      auto model = util::findFile(base_path, "");
      parameters.put("model", model.string());
    } else {
      parameters.put("model", (base_path / config.id).string());
    }
    i++;
  }
}

size_t ModelConfig::size() const { return configs_.size(); }

std::pair<std::string, ParameterMap> ModelConfig::get(size_t index) {
  return models_.at(index);
}

ModelConfig::Iterator ModelConfig::begin() { return models_.begin(); }
ModelConfig::ConstIterator ModelConfig::begin() const {
  return models_.cbegin();
}
ModelConfig::ConstIterator ModelConfig::cbegin() const {
  return models_.cbegin();
}
ModelConfig::ReverseIterator ModelConfig::rbegin() { return models_.rbegin(); }
ModelConfig::ConstReverseIterator ModelConfig::rbegin() const {
  return models_.crbegin();
}
ModelConfig::ConstReverseIterator ModelConfig::crbegin() const {
  return models_.crbegin();
}

ModelConfig::Iterator ModelConfig::end() { return models_.end(); }
ModelConfig::ConstIterator ModelConfig::end() const { return models_.cend(); }
ModelConfig::ConstIterator ModelConfig::cend() const { return models_.cend(); }
ModelConfig::ReverseIterator ModelConfig::rend() { return models_.rend(); }
ModelConfig::ConstReverseIterator ModelConfig::rend() const {
  return models_.crend();
}
ModelConfig::ConstReverseIterator ModelConfig::crend() const {
  return models_.crend();
}

}  // namespace amdinfer
