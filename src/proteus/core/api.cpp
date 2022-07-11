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

#include "proteus/batching/batcher.hpp"
#include "proteus/core/interface.hpp"
#include "proteus/core/manager.hpp"
#include "proteus/core/model_repository.hpp"
#include "proteus/core/predict_api.hpp"
#include "proteus/core/worker_info.hpp"
#include "proteus/version.hpp"  // for kProteusVersion

namespace proteus {

ServerMetadata serverMetadata() {
  std::unordered_set<std::string> extensions;
  ServerMetadata metadata{"proteus", kProteusVersion, extensions};

#ifdef PROTEUS_ENABLE_AKS
  metadata.extensions.emplace("aks");
#endif
#ifdef PROTEUS_ENABLE_VITIS
  metadata.extensions.emplace("vitis");
#endif
#ifdef PROTEUS_ENABLE_TFZENDNN
  metadata.extensions.emplace("tfzendnn");
#endif
#ifdef PROTEUS_ENABLE_PTZENDNN
  metadata.extensions.emplace("ptzendnn");
#endif
#ifdef PROTEUS_ENABLE_MIGRAPHX
  metadata.extensions.emplace("migraphx");
#endif
  return metadata;
}

void modelLoad(const std::string& model, RequestParameters* parameters) {
  assert(parameters != nullptr);
  for (const auto& c : model) {
    assert(c == std::tolower(c));
  }

  ModelRepository::modelLoad(model, parameters);
  Manager::getInstance().loadWorker(model, *parameters);
}

void modelUnload(const std::string& model) { workerUnload(model); }

std::string workerLoad(const std::string& worker,
                       RequestParameters* parameters) {
  assert(parameters != nullptr);
  for (const auto& c : worker) {
    assert(c == std::tolower(c));
  }

  parameters->put("worker", worker);
  return Manager::getInstance().loadWorker(worker, *parameters);
}

void workerUnload(const std::string& worker) {
  for (const auto& c : worker) {
    assert(c == std::tolower(c));
  }
  Manager::getInstance().unloadWorker(worker);
}

void modelInfer(const std::string& model, std::unique_ptr<Interface> request) {
  WorkerInfo* worker = Manager::getInstance().getWorker(model);
  if (worker == nullptr) {
    throw invalid_argument("Worker " + model + " not found");
  }
  auto* batcher = worker->getBatcher();
  batcher->enqueue(std::move(request));
}

bool modelReady(const std::string& model) {
  return Manager::getInstance().workerReady(model);
}

std::vector<std::string> modelList() {
  return Manager::getInstance().getWorkerEndpoints();
}

ModelMetadata modelMetadata(const std::string& model) {
  return Manager::getInstance().getWorkerMetadata(model);
}

}  // namespace proteus
