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
#include <stdexcept>      // for invalid_argument
#include <string>         // for string, basic_string
#include <unordered_map>  // for unordered_map, operat...
#include <unordered_set>  // for unordered_set
#include <utility>        // for move, pair, make_pair

#include "proteus/batching/batcher.hpp"         // for Batcher
#include "proteus/build_options.hpp"            // for PROTEUS_ENABLE_TRACING
#include "proteus/clients/native_internal.hpp"  // for CppNativeApi
#include "proteus/core/api.hpp"                 // for modelLoad
#include "proteus/core/exceptions.hpp"          // for bad_status
#include "proteus/core/manager.hpp"             // for Manager
#include "proteus/core/worker_info.hpp"         // for WorkerInfo
#include "proteus/helpers/exec.hpp"             // for exec
#include "proteus/helpers/string.hpp"           // for toLower
#include "proteus/observation/logging.hpp"      // for initLogging
#include "proteus/observation/metrics.hpp"      // for Metrics, MetricCounte...
#include "proteus/observation/tracing.hpp"      // for startTrace, startTracer
#include "proteus/version.hpp"                  // for kProteusVersion

namespace proteus {

ServerMetadata NativeClient::serverMetadata() {
  return ::proteus::serverMetadata();
}
bool NativeClient::serverLive() { return true; }
bool NativeClient::serverReady() { return true; }

ModelMetadata NativeClient::modelMetadata(const std::string& model) {
  return ::proteus::modelMetadata(model);
}

void NativeClient::modelLoad(const std::string& model,
                             RequestParameters* parameters) {
  auto model_lower = toLower(model);
  if (parameters == nullptr) {
    RequestParameters params;
    ::proteus::modelLoad(model_lower, &params);
  } else {
    ::proteus::modelLoad(model_lower, parameters);
  }
}

std::string NativeClient::workerLoad(const std::string& model,
                                     RequestParameters* parameters) {
  auto model_lower = toLower(model);
  if (parameters == nullptr) {
    RequestParameters params;
    return ::proteus::workerLoad(model_lower, &params);
  }
  return ::proteus::workerLoad(model_lower, parameters);
}

InferenceResponseFuture NativeClient::enqueue(const std::string& workerName,
                                              InferenceRequest request) {
#ifdef PROTEUS_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::kCppNative);
#endif

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
  ::proteus::modelInfer(workerName, std::move(api));

  return future;
}

InferenceResponse NativeClient::modelInfer(const std::string& model,
                                           const InferenceRequest& request) {
  auto future = enqueue(model, request);
  return future.get();
}

void NativeClient::modelUnload(const std::string& model) {
  auto model_lower = toLower(model);
  ::proteus::modelUnload(model);
}

void NativeClient::workerUnload(const std::string& model) {
  auto model_lower = toLower(model);
  ::proteus::workerUnload(model);
}

bool NativeClient::modelReady(const std::string& model) {
  try {
    return ::proteus::modelReady(model);
  } catch (const invalid_argument&) {
    return false;
  }
}

std::vector<std::string> NativeClient::modelList() {
  return ::proteus::modelList();
}

bool NativeClient::hasHardware(const std::string& name, int num) {
  return ::proteus::hasHardware(name, num);
}

}  // namespace proteus
