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

#ifndef GUARD_MLCOMMONS_SRC_CONFIG_PARSER
#define GUARD_MLCOMMONS_SRC_CONFIG_PARSER

#include <string>

#include "amdinfer/core/exceptions.hpp"
#include "amdinfer/core/parameters.hpp"

namespace amdinfer {

class Config {
 public:
  template <typename T>
  void put(const std::string& key, const T& value) {
    config_.put(key, value);
  }

  template <typename T>
  T get(const std::string& model, const std::string& scenario,
        const std::string& key) {
    if (config_.has(model + "." + scenario + "." + key)) {
      return config_.get<T>(model + "." + scenario + "." + key);
    }
    if (config_.has("*." + scenario + "." + key)) {
      return config_.get<T>("*." + scenario + "." + key);
    }
    if (config_.has(model + ".*." + key)) {
      return config_.get<T>(model + ".*." + key);
    }
    if (config_.has("*.*." + key)) {
      return config_.get<T>("*.*." + key);
    }
    throw invalid_argument("Key not found");
  }

  bool has(const std::string& model, const std::string& scenario,
           const std::string& key) {
    if (config_.has(model + "." + scenario + "." + key)) {
      return true;
    }
    if (config_.has("*." + scenario + "." + key)) {
      return true;
    }
    if (config_.has(model + ".*." + key)) {
      return true;
    }
    if (config_.has("*.*." + key)) {
      return true;
    }
    return false;
  }

  ParameterMap getParameters(const std::string& model,
                             const std::string& scenario);

 private:
  ParameterMap config_;
};

Config parseConfig(const std::string& path);

}  // namespace amdinfer

#endif  // GUARD_MLCOMMONS_SRC_CONFIG_PARSER
