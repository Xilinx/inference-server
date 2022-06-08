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

#include <cstdlib>        // for getenv
#include <future>         // for promise
#include <memory>         // for unique_ptr, make_unique
#include <set>            // for set
#include <stdexcept>      // for invalid_argument
#include <string>         // for string, basic_string
#include <unordered_map>  // for unordered_map, operat...
#include <utility>        // for move, pair, make_pair

#include "proteus/batching/batcher.hpp"         // for Batcher
#include "proteus/build_options.hpp"            // for PROTEUS_ENABLE_TRACING
#include "proteus/clients/native_internal.hpp"  // for CppNativeApi
#include "proteus/core/manager.hpp"             // for Manager
#include "proteus/core/worker_info.hpp"         // for WorkerInfo
#include "proteus/helpers/exec.hpp"             // for exec
#include "proteus/observation/logging.hpp"      // for initLogging
#include "proteus/observation/metrics.hpp"      // for Metrics, MetricCounte...
#include "proteus/observation/tracing.hpp"      // for startTrace, startTracer
#include "proteus/version.hpp"                  // for kProteusVersion

#ifdef PROTEUS_ENABLE_AKS
#include <aks/AksSysManagerExt.h>  // for SysManagerExt
#endif

namespace proteus {

std::string getLogDirectory() {
  auto* home = std::getenv("HOME");
  std::string dir;
  if (home != nullptr) {
    dir = home;
    dir += "/.proteus";
  } else {
    dir = ".";
  }
  dir += "/logs";
  return dir;
}

void initializeLogging() {
#ifdef PROTEUS_ENABLE_LOGGING
  LogOptions options{
    "server",  // logger_name
    getLogDirectory(),
    true,              // enable file logging
    LogLevel::kDebug,  // file log level
    true,              // enable console logging
    LogLevel::kWarn    // console log level
  };
  initLogger(options);

  options.logger_name = "client";
  initLogger(options);
#endif
}

void initialize() {
  initializeLogging();
#ifdef PROTEUS_ENABLE_TRACING
  startTracer();
#endif

  Manager::getInstance().init();

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

NativeClient::~NativeClient() = default;

ServerMetadata NativeClient::serverMetadata() {
  std::set<std::string, std::less<>> extensions;
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
  metadata.extensions.insert("ptzendnn");
#endif
  return metadata;
}
bool NativeClient::serverLive() { return true; }
bool NativeClient::serverReady() { return true; }

void NativeClient::modelLoad(const std::string& model,
                             RequestParameters* parameters) {
  if (parameters == nullptr) {
    Manager::getInstance().loadWorker(model, RequestParameters());
  } else {
    Manager::getInstance().loadWorker(model, *parameters);
  }
}

std::string NativeClient::workerLoad(const std::string& model,
                                     RequestParameters* parameters) {
  if (parameters == nullptr) {
    return Manager::getInstance().loadWorker(model, RequestParameters());
  }
  return Manager::getInstance().loadWorker(model, *parameters);
}

InferenceResponseFuture NativeClient::enqueue(const std::string& workerName,
                                              InferenceRequest request) {
#ifdef PROTEUS_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::kCppNative);
#endif
  auto* worker = proteus::Manager::getInstance().getWorker(workerName);

#ifdef PROTEUS_ENABLE_TRACING
  auto trace = startTrace(&(__func__[0]));
  trace->startSpan("C++ enqueue");
#endif
  auto api = std::make_unique<CppNativeApi>(std::move(request));
  auto future = api->getPromise()->get_future();
#ifdef PROTEUS_ENABLE_TRACING
  trace->endSpan();
  api->setTrace(std::move(trace));
#endif
  worker->getBatcher()->enqueue(std::move(api));
  // this->cv_.notify_one();
  return future;
}

InferenceResponse NativeClient::modelInfer(const std::string& model,
                                           const InferenceRequest& request) {
  auto future = enqueue(model, request);
  return future.get();
}

void NativeClient::modelUnload(const std::string& model) {
  Manager::getInstance().unloadWorker(model);
}

void NativeClient::workerUnload(const std::string& model) {
  Manager::getInstance().unloadWorker(model);
}

bool NativeClient::modelReady(const std::string& model) {
  return Manager::getInstance().workerReady(model);
}

std::vector<std::string> NativeClient::modelList() {
  return Manager::getInstance().getWorkerEndpoints();
}

std::string getHardware() {
  std::string hardware;
#ifdef PROTEUS_ENABLE_VITIS
  hardware = exec("fpga-util get-kernels");
  if (!hardware.empty()) {
    hardware.pop_back();  // remove trailing newline character
  }
#endif
  return hardware;
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
    if (size_t found = token.find(':'); found != std::string::npos) {
      auto found_kernel = token.substr(0, found);
      size_t found_num = 0;
      size_t idx = 0;
      auto substr = token.substr(found + 1, std::string::npos);
      try {
        found_num = std::stoi(substr, &idx);
      } catch (const std::invalid_argument&) {
        // if the substring can't be converted to an integer
        found_num = 0;
      }
      if (idx != substr.length()) {
        // if there are non-numeric characters in the "number"
        found_num = 0;
      }
      kernels.try_emplace(found_kernel, found_num);
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
