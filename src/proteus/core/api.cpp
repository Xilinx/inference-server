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

#include <json/reader.h>  // for CharReaderBuilder
#include <json/value.h>

#include <algorithm>
#include <string>

#include "proteus/batching/batcher.hpp"
#include "proteus/build_options.hpp"
#include "proteus/core/interface.hpp"
#include "proteus/core/manager.hpp"
#include "proteus/core/model_repository.hpp"
#include "proteus/core/predict_api.hpp"
#include "proteus/core/worker_info.hpp"
#include "proteus/version.hpp"  // for kProteusVersion

#ifdef PROTEUS_ENABLE_VITIS
#include <sockpp/tcp_connector.h>
#endif

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

Kernels getHardware() {
  Kernels kernels;

#ifdef PROTEUS_ENABLE_VITIS
  sockpp::socket_initializer sockInit;
  sockpp::tcp_connector conn({"localhost", 9763});
  if (!conn) {
    std::cerr << "Error connecting to server"
              << "\n\t" << conn.last_error_str() << std::endl;
  }

  std::string request("{\"request\":{\"name\":\"list\",\"requestId\":\"1\"}}");

  auto bar = conn.write(request);
  if (bar == -1) {
    throw external_error("Bytes no match");
  }

  int total_len = 0;
  auto len_read = conn.read_n(&total_len, 4);
  if (len_read == -1) {
    throw external_error("wrong number of bytes read initially");
  }
  std::string response_str;
  response_str.resize(total_len);
  auto foo = conn.read_n(response_str.data(), total_len);
  if (foo == -1) {
    throw external_error("wrong number of bytes read at data");
  }

  std::string errors;
  Json::CharReaderBuilder builder;
  Json::CharReader* reader = builder.newCharReader();
  Json::Value response;
  reader->parse(response_str.c_str(),
                response_str.data() + response_str.length(), &response,
                &errors);

  auto data = response["response"]["data"];
  auto num_fpgas = std::stoi(data["deviceNumber"].asString());
  for (auto i = 0; i < num_fpgas; ++i) {
    auto device = data["device_" + std::to_string(i)];
    if (!device.isMember("cuNumber   ")) {
      continue;
    }
    auto cu_num = std::stoi(device["cuNumber   "].asString());
    for (auto j = 0; j < cu_num; ++j) {
      auto kernel =
        device["cu_" + std::to_string(j)]["kernelName   "].asString();
      if (kernels.find(kernel) == kernels.end()) {
        kernels.try_emplace(kernel, 1);
      } else {
        kernels.at(kernel)++;
      }
    }
  }

#endif

  return kernels;
}

bool hasHardware(const std::string& name, int num) {
  auto kernels = getHardware();

  auto foo = kernels.find(name);
  if (foo == kernels.end()) {
    return false;
  }
  return foo->second >= num;
}

}  // namespace proteus
