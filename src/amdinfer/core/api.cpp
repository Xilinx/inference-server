// Copyright 2022 Xilinx, Inc.
// Copyright 2022 Advanced Micro Devices, Inc.
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

#include "amdinfer/core/api.hpp"

#include <json/reader.h>  // for CharReaderBuilder, Char...
#include <json/value.h>   // for Value

#include <cassert>        // for assert
#include <cctype>         // for tolower
#include <iostream>       // for operator<<, basic_ostream
#include <string>         // for string, operator+, stoi
#include <unordered_map>  // for operator==, unordered_m...
#include <unordered_set>  // for unordered_set
#include <utility>        // for move, pair

#include "amdinfer/batching/batcher.hpp"       // for Batcher
#include "amdinfer/build_options.hpp"          // for AMDINFER_ENABLE_VITIS
#include "amdinfer/core/exceptions.hpp"        // for external_error, invalid...
#include "amdinfer/core/interface.hpp"         // IWYU pragma: keep
#include "amdinfer/core/manager.hpp"           // for Manager
#include "amdinfer/core/model_repository.hpp"  // for ModelRepository
#include "amdinfer/core/predict_api.hpp"       // for ServerMetadata, ModelMe...
#include "amdinfer/core/worker_info.hpp"       // for WorkerInfo
#include "amdinfer/version.hpp"                // for kAmdinferVersion

#ifdef AMDINFER_ENABLE_VITIS
#include <sockpp/socket.h>         // for socket, socket_initializer
#include <sockpp/tcp_connector.h>  // for tcp_connector
#endif

namespace amdinfer {

ServerMetadata serverMetadata() {
  std::unordered_set<std::string> extensions;
  ServerMetadata metadata{"amdinfer", kAmdinferVersion, extensions};

#ifdef AMDINFER_ENABLE_AKS
  metadata.extensions.emplace("aks");
#endif
#ifdef AMDINFER_ENABLE_VITIS
  metadata.extensions.emplace("vitis");
#endif
#ifdef AMDINFER_ENABLE_TFZENDNN
  metadata.extensions.emplace("tfzendnn");
#endif
#ifdef AMDINFER_ENABLE_PTZENDNN
  metadata.extensions.emplace("ptzendnn");
#endif
#ifdef AMDINFER_ENABLE_MIGRAPHX
  metadata.extensions.emplace("migraphx");
#endif
  return metadata;
}

void modelLoad(const std::string& model, ParameterMap* parameters) {
  assert(parameters != nullptr);
  for (const auto& c : model) {
    assert(c == std::tolower(c));
  }

  parseModel("/workspace/amdinfer/external/artifacts/repository", model,
             parameters);
  Manager::getInstance().loadWorker(model, *parameters);
}

void modelUnload(const std::string& model) { workerUnload(model); }

std::string workerLoad(const std::string& worker, ParameterMap* parameters) {
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

#ifdef AMDINFER_ENABLE_VITIS
  sockpp::socket_initializer sock_init;
  const auto default_xrm_port = 9763;
  sockpp::tcp_connector conn({"localhost", default_xrm_port});
  if (!conn) {
    std::cerr << "Error connecting to server"
              << "\n\t" << conn.last_error_str() << std::endl;
  }

  std::string request(R"({"request":{"name":"list","requestId":"1"}})");

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
  auto bytes_read = conn.read_n(response_str.data(), total_len);
  if (bytes_read == -1) {
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

  auto kernel_iterator = kernels.find(name);
  if (kernel_iterator == kernels.end()) {
    return false;
  }
  return kernel_iterator->second >= num;
}

}  // namespace amdinfer
