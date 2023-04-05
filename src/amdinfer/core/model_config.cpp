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
#include "model_config.pb.h"  // for Config, InferP...

namespace fs = std::filesystem;

namespace amdinfer {

std::string extractString(const toml::table& table, const std::string& key,
                          bool is_required) {
  if (!table.contains(key)) {
    if (is_required) {
      throw invalid_argument("The configuration must have a " + key);
    } else {
      return "";
    }
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
constexpr std::false_type templated_false{};

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
      static_assert(templated_false<T>);
    }
  }

  return vector;
}

ModelConfigTensor extractModelConfigTensor(const toml::table& table) {
  auto name = extractString(table, "name", true);
  auto datatype_str = extractString(table, "datatype", true);
  auto shape = extractArray<uint64_t>(table, "shape");
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
                                     std::vector<uint64_t> shape,
                                     DataType data_type, std::string id)
  : Tensor(std::move(name), std::move(shape), data_type), id_(std::move(id)) {}

const std::string& ModelConfigTensor::id() const& { return id_; }

std::string ModelConfigTensor::id() && { return std::move(id_); }

ModelConfig::ModelConfig(const toml::table& toml) {
  if (toml.empty()) {
    throw invalid_argument("The configuration cannot be empty");
  }

  if (toml.contains("models")) {
    configs_ = extractArray<ModelConfigData>(toml, "models");
  } else {
    configs_.emplace_back(extractConfig(toml, false));
  }
}

ModelConfig::ModelConfig(const inference::Config& config) {
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
    std::vector<uint64_t> shape;
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
    std::vector<uint64_t> shape;
    shape.reserve(proto_shape.size());
    for (const auto& index : proto_shape) {
      shape.push_back(index);
    }
    outputs.emplace_back(name, shape, datatype, id);
  }

  configs_.emplace_back(model_name, platform, id, inputs, outputs);
}

size_t ModelConfig::size() const { return configs_.size(); }

const std::string& ModelConfig::name(size_t index) const {
  return configs_.at(index).name;
}
const std::string& ModelConfig::platform(size_t index) const {
  return configs_.at(index).platform;
}
const std::string& ModelConfig::id(size_t index) const {
  return configs_.at(index).id;
}
const std::vector<ModelConfigTensor>& ModelConfig::inputs(size_t index) const {
  return configs_.at(index).inputs;
}
const std::vector<ModelConfigTensor>& ModelConfig::outputs(size_t index) const {
  return configs_.at(index).outputs;
}

}  // namespace amdinfer
