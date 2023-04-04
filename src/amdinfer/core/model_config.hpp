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

class ModelConfig {
 public:
  explicit ModelConfig(const toml::v3::table& config);
  explicit ModelConfig(const inference::Config& config);

  const std::string& name() const;
  const std::string& platform() const;
  const std::vector<Tensor>& inputs() const;
  const std::vector<Tensor>& outputs() const;

 private:
  std::string name_;
  std::string platform_;
  std::vector<Tensor> inputs_;
  std::vector<Tensor> outputs_;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_MODEL_CONFIG
