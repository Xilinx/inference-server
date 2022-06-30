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
 * @brief Implements the client-server API used in the inference server
 */

#include "proteus/core/api.hpp"

#include <algorithm>
#include <string>

#include "proteus/core/manager.hpp"
#include "proteus/core/model_repository.hpp"
#include "proteus/core/predict_api.hpp"
#include "proteus/core/worker_info.hpp"

namespace proteus {

void modelLoad(const std::string& model, RequestParameters* parameters) {
  assert(parameters != nullptr);
  for (const auto& c : model) {
    assert(c == std::tolower(c));
  }

  ModelRepository::modelLoad(model, parameters);
  Manager::getInstance().loadWorker(model, *parameters);
}

void modelUnload(const std::string& model) { workerUnload(model); }

std::string workerLoad(const std::string& model,
                       RequestParameters* parameters) {
  assert(parameters != nullptr);
  for (const auto& c : model) {
    assert(c == std::tolower(c));
  }

  parameters->put("worker", model);
  return Manager::getInstance().loadWorker(model, *parameters);
}

void workerUnload(const std::string& model) {
  for (const auto& c : model) {
    assert(c == std::tolower(c));
  }
  Manager::getInstance().unloadWorker(model);
}

}  // namespace proteus
