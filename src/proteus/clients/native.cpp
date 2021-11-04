// Copyright 2021 Xilinx Inc.
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
 * @brief Implements the methods for interacting with Proteus in the native C++
 * API
 */

#include "proteus/clients/native.hpp"

#include <cstdlib>        // for getenv, size_t
#include <memory>         // for unique_ptr
#include <stdexcept>      // for invalid_argument
#include <string>         // for string, basic_string, all...
#include <thread>         // for thread
#include <unordered_map>  // for unordered_map, operator==
#include <utility>        // for pair, make_pair, move

#include "proteus/batching/batcher.hpp"     // for Batcher
#include "proteus/build_options.hpp"        // for PROTEUS_ENABLE_TRACING
#include "proteus/core/manager.hpp"         // for Manager
#include "proteus/core/predict_api.hpp"     // for InferenceRequestInput
#include "proteus/core/worker_info.hpp"     // for WorkerInfo
#include "proteus/helpers/exec.hpp"         // for exec
#include "proteus/observation/logging.hpp"  // for initLogging
#include "proteus/observation/metrics.hpp"  // for Metrics
#include "proteus/observation/tracing.hpp"  // for startTracer, stopTracer
#include "proteus/servers/http_server.hpp"  // for start

#ifdef PROTEUS_ENABLE_AKS
#include <aks/AksSysManagerExt.h>  // for SysManagerExt
#endif

namespace proteus {

void initialize() {
#ifdef PROTEUS_ENABLE_LOGGING
  initLogging();
#endif
#ifdef PROTEUS_ENABLE_TRACING
  startTracer("/workspace/proteus/src/proteus/observation/tracing.yml");
#endif

#ifdef PROTEUS_ENABLE_AKS
  auto* aks_sys_manager = AKS::SysManagerExt::getGlobal();

  // Load all the kernels
  aks_sys_manager->loadKernels(std::string(std::getenv("AKS_ROOT")) +
                               std::string("/kernel_zoo"));
#endif
}

void terminate() {
#ifdef PROTEUS_ENABLE_TRACING
  stopTracer();
#endif
  Manager::getInstance().shutdown();

#ifdef PROTEUS_ENABLE_AKS
  AKS::SysManagerExt::deleteGlobal();
#endif
}

void startHttpServer(int port) {
#ifdef PROTEUS_ENABLE_HTTP
  std::thread{proteus::http::start, port}.detach();
#else
  (void)port;  // suppress unused variable warning
#endif
}

void stopHttpServer() {
#ifdef PROTEUS_ENABLE_HTTP
  proteus::http::stop();
#endif
}

std::string load(const std::string& worker, RequestParameters* parameters) {
  // FIXME(varunsh): parameters can't be null to loadWorker
  return Manager::getInstance().loadWorker(worker, parameters);
}

InferenceResponseFuture enqueue(const std::string& workerName,
                                InferenceRequestInput request) {
#ifdef PROTEUS_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricIDs::kCounterCppNative);
#endif
  auto* worker = proteus::Manager::getInstance().getWorker(workerName);
  return worker->getBatcher()->enqueue(std::move(request));
}

std::string getHardware() {
  auto* env = std::getenv("PROTEUS_ROOT");
  if (env != nullptr) {
    auto kernels = exec("fpga-util get-kernels");
    if (!kernels.empty()) {
      kernels.pop_back();  // remove trailing newline character
    }
    return kernels;
  }
  return "";
}

bool hasHardware(const std::string& kernel, size_t num) {
  // hw is of the form "kernel0:num0[,kernel1:num1,...]"
  auto hw = getHardware();
  std::string delimiter = ",";

  std::unordered_map<std::string, size_t> kernels;
  size_t pos = 0;
  size_t max_kernel_num = 0;
  do {
    pos = hw.find(delimiter);
    auto token = hw.substr(0, pos);
    size_t found = std::string::npos;
    if ((found = token.find(':')) != std::string::npos) {
      auto found_kernel = token.substr(0, found);
      size_t found_num = 0;
      size_t idx = 0;
      auto substr = token.substr(found + 1, std::string::npos);
      try {
        found_num = std::stoi(substr, &idx);
      } catch (const std::invalid_argument& e) {
        // if the substring can't be converted to an integer
        found_num = 0;
      }
      if (idx != substr.length()) {
        // if there are non-numeric characters in the "number"
        found_num = 0;
      }
      kernels.insert(std::make_pair(found_kernel, found_num));
      if (found_num > max_kernel_num) {
        max_kernel_num = found_num;
      }
    }
    hw.erase(0, pos + delimiter.length());
  } while (pos != std::string::npos);

  // For the special case of an empty kernel string, return if there are any
  // kernels that exceed the requested number. This allows you to match if there
  // is any one type of kernel in sufficient number.
  if (kernel.empty()) {
    return max_kernel_num >= num;
  }

  if (kernels.find(kernel) == kernels.end()) {
    return false;
  }
  return kernels[kernel] >= num;
}

}  // namespace proteus
