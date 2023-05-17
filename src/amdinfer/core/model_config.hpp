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

#ifndef GUARD_AMDINFER_CORE_MODEL_CONFIG
#define GUARD_AMDINFER_CORE_MODEL_CONFIG

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "amdinfer/core/parameters.hpp"
#include "amdinfer/core/tensor.hpp"

namespace toml {
inline namespace v3 {
class table;
}  // namespace v3
}  // namespace toml

namespace inference {
class Config;
}  // namespace inference

namespace amdinfer {

class ModelConfigTensor : public Tensor {
 public:
  ModelConfigTensor(std::string name, std::vector<uint64_t> shape,
                    DataType data_type, std::string id);

  const std::string& id() const&;
  std::string id() &&;

 private:
  std::string id_;
};

struct ModelConfigData {
  ModelConfigData(std::string name, std::string platform, std::string id,
                  std::vector<ModelConfigTensor> inputs,
                  std::vector<ModelConfigTensor> outputs)
    : name(std::move(name)),
      platform(std::move(platform)),
      id(std::move(id)),
      inputs(std::move(inputs)),
      outputs(std::move(outputs)) {}

  std::string name;
  std::string platform;
  std::string id;
  std::vector<ModelConfigTensor> inputs;
  std::vector<ModelConfigTensor> outputs;
};

class ModelConfig {
  using Container = std::vector<std::pair<std::string, ParameterMap>>;
  using Iterator = Container::iterator;
  using ConstIterator = Container::const_iterator;
  using ReverseIterator = Container::reverse_iterator;
  using ConstReverseIterator = Container::const_reverse_iterator;

 public:
  ModelConfig(const toml::v3::table& config, const std::string& version);
  ModelConfig(const inference::Config& config, const std::string& version);

  std::pair<std::string, ParameterMap> get(size_t index);
  void setModelFiles(const std::filesystem::path& base_path);

  size_t size() const;

  /// Returns a read/write iterator to the first parameter in the object
  Iterator begin();
  /// Returns a read iterator to the first parameter in the object
  [[nodiscard]] ConstIterator begin() const;
  /// Returns a read iterator to the first parameter in the object
  [[nodiscard]] ConstIterator cbegin() const;
  /// Returns a read/write iterator to the first parameter in the object
  ReverseIterator rbegin();
  /// Returns a read iterator to the first parameter in the object
  [[nodiscard]] ConstReverseIterator rbegin() const;
  /// Returns a read iterator to the first parameter in the object
  [[nodiscard]] ConstReverseIterator crbegin() const;

  /// Returns a read/write iterator to one past the last parameter in the object
  Iterator end();
  /// Returns a read iterator to one past the last parameter in the object
  [[nodiscard]] ConstIterator end() const;
  /// Returns a read iterator to one past the last parameter in the object
  [[nodiscard]] ConstIterator cend() const;
  /// Returns a read/write iterator to the first parameter in the object
  ReverseIterator rend();
  /// Returns a read iterator to the first parameter in the object
  [[nodiscard]] ConstReverseIterator rend() const;
  /// Returns a read iterator to the first parameter in the object
  [[nodiscard]] ConstReverseIterator crend() const;

 private:
  std::vector<ModelConfigData> configs_;
  Container models_;

  void createModels();
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_MODEL_CONFIG
