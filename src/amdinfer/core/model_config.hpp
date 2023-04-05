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

#include <amdinfer/core/tensor.hpp>
#include <string>
#include <vector>

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
 public:
  explicit ModelConfig(const toml::v3::table& config);
  explicit ModelConfig(const inference::Config& config);

  const std::string& name(size_t index = 0) const;
  const std::string& platform(size_t index = 0) const;
  const std::string& id(size_t index = 0) const;
  const std::vector<ModelConfigTensor>& inputs(size_t index = 0) const;
  const std::vector<ModelConfigTensor>& outputs(size_t index = 0) const;

 private:
  std::vector<ModelConfigData> configs_;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_MODEL_CONFIG
