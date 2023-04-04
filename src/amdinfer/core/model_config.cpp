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

Tensor extractTensor(const toml::table& table);

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
    } else if constexpr (std::is_same_v<T, Tensor>) {
      if (!element.is_table()) {
        throw invalid_argument(
          "One or more values could not be converted to a table");
      }
      const auto& tbl = *element.as_table();
      vector.push_back(extractTensor(tbl));
    } else {
      static_assert(templated_false<T>);
    }
  }

  return vector;
}

Tensor extractTensor(const toml::table& table) {
  auto name = extractString(table, "name", true);
  auto datatype_str = extractString(table, "datatype", true);
  auto shape = extractArray<uint64_t>(table, "shape");

  DataType datatype{datatype_str.c_str()};

  return {name, shape, datatype};
}

ModelConfig::ModelConfig(const toml::table& toml) {
  if (toml.empty()) {
    throw invalid_argument("The configuration cannot be empty");
  }

  name_ = extractString(toml, "name", true);
  platform_ = extractString(toml, "platform", true);

  inputs_ = extractArray<Tensor>(toml, "inputs");
  outputs_ = extractArray<Tensor>(toml, "outputs");
}

ModelConfig::ModelConfig(const inference::Config& config)
  : name_(config.name()), platform_(config.platform()) {
  const auto& proto_inputs = config.inputs();
  for (const auto& input : proto_inputs) {
    const auto& name = input.name();
    const DataType datatype{input.datatype().c_str()};
    const auto& proto_shape = input.shape();
    std::vector<uint64_t> shape;
    shape.reserve(proto_shape.size());
    for (const auto& index : proto_shape) {
      shape.push_back(index);
    }
    inputs_.emplace_back(name, shape, datatype);
  }

  const auto& proto_outputs = config.outputs();
  for (const auto& output : proto_outputs) {
    const auto& name = output.name();
    const DataType datatype{output.datatype().c_str()};
    const auto& proto_shape = output.shape();
    std::vector<uint64_t> shape;
    shape.reserve(proto_shape.size());
    for (const auto& index : proto_shape) {
      shape.push_back(index);
    }
    outputs_.emplace_back(name, shape, datatype);
  }
}

const std::string& ModelConfig::name() const { return name_; }
const std::string& ModelConfig::platform() const { return platform_; }
const std::vector<Tensor>& ModelConfig::inputs() const { return inputs_; }
const std::vector<Tensor>& ModelConfig::outputs() const { return outputs_; }

}  // namespace amdinfer
