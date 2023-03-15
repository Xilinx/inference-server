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

#include "amdinfer/core/shared_state.hpp"

#include <jsoncpp/json/reader.h>  // for CharReaderBuilder, Char...
#include <jsoncpp/json/value.h>   // for Value

#include <cassert>        // for assert
#include <filesystem>     // for path
#include <iostream>       // for operator<<, basic_ostream
#include <string>         // for string, operator+, stoi
#include <unordered_map>  // for operator==, unordered_m...
#include <unordered_set>  // for unordered_set
#include <utility>        // for move, pair

#include "amdinfer/build_options.hpp"          // for AMDINFER_ENABLE_VITIS
#include "amdinfer/core/endpoints.hpp"         // for Endpoints
#include "amdinfer/core/exceptions.hpp"        // for external_error, invalid...
#include "amdinfer/core/model_repository.hpp"  // for ModelRepository
#include "amdinfer/core/parameters.hpp"        // for ParameterMap
#include "amdinfer/core/predict_api_internal.hpp"  // for ServerMetadata, ModelMe...
#include "amdinfer/observation/observer.hpp"
#include "amdinfer/util/string.hpp"  // for isLower
#include "amdinfer/version.hpp"      // for kAmdinferVersion

#ifdef AMDINFER_ENABLE_VITIS
#include <sockpp/socket.h>         // for socket, socket_initializer
#include <sockpp/tcp_connector.h>  // for tcp_connector
#endif

namespace fs = std::filesystem;

namespace amdinfer {

ServerMetadata SharedState::serverMetadata() {
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

void SharedState::modelLoad(const std::string& model,
                            const ParameterMap& parameters) {
  assert(util::isLower(model));

  ParameterMap updated_parameters = parameters;

  parseModel(repository_.getRepository(), model, &updated_parameters);
  endpoints_.load(model, updated_parameters);
}

void SharedState::modelUnload(const std::string& model) {
  this->workerUnload(model);
}

std::string SharedState::workerLoad(const std::string& worker,
                                    const ParameterMap& parameters) {
  assert(util::isLower(worker));

  auto updated_parameters = parameters;
  updated_parameters.put("worker", worker);
  return endpoints_.load(worker, updated_parameters);
}

void SharedState::workerUnload(const std::string& worker) {
  assert(util::isLower(worker));
  endpoints_.unload(worker);
}

void SharedState::modelInfer(const std::string& model,
                             std::unique_ptr<RequestContainer> request) {
  endpoints_.infer(model, std::move(request));
}

bool SharedState::modelReady(const std::string& model) {
  return endpoints_.ready(model);
}

std::vector<std::string> SharedState::modelList() { return endpoints_.list(); }

ModelMetadata SharedState::modelMetadata(const std::string& model) {
  return endpoints_.metadata(model);
}

Kernels SharedState::getHardware() {
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

bool SharedState::hasHardware(const std::string& name, int num) {
  auto kernels = SharedState::getHardware();

  auto kernel_iterator = kernels.find(name);
  if (kernel_iterator == kernels.end()) {
    return false;
  }
  return kernel_iterator->second >= num;
}

const MemoryPool* SharedState::getPool() const { return endpoints_.getPool(); }

void SharedState::setRepository(const fs::path& repository_path,
                                bool load_existing) {
  repository_.setEndpoints(&endpoints_);
  repository_.setRepository(repository_path, load_existing);
}

void SharedState::enableRepositoryMonitoring(bool use_polling) {
  repository_.enableMonitoring(use_polling);
}

}  // namespace amdinfer
