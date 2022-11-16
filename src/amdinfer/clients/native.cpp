// Copyright 2021 Xilinx, Inc.
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
 * @brief Implements the methods for interacting with the server in the native
 * C++ API
 */

#include "amdinfer/clients/native.hpp"

#include <future>   // for future, promise
#include <memory>   // for unique_ptr, make_unique
#include <string>   // for string
#include <utility>  // for move

#include "amdinfer/build_options.hpp"            // for AMDINFER_ENABLE_TRACING
#include "amdinfer/clients/native_internal.hpp"  // for CppNativeApi
#include "amdinfer/core/api.hpp"                 // for modelLoad, workerLoad
#include "amdinfer/core/exceptions.hpp"          // for invalid_argument
#include "amdinfer/core/interface.hpp"           // for Interface
#include "amdinfer/observation/metrics.hpp"      // for Metrics, MetricCounte...
#include "amdinfer/observation/tracing.hpp"      // for startTrace, Trace
#include "amdinfer/util/string.hpp"              // for toLower

namespace amdinfer {

ServerMetadata NativeClient::serverMetadata() const {
  return ::amdinfer::serverMetadata();
}
bool NativeClient::serverLive() const { return true; }
bool NativeClient::serverReady() const { return true; }

ModelMetadata NativeClient::modelMetadata(const std::string& model) const {
  return ::amdinfer::modelMetadata(model);
}

void NativeClient::modelLoad(const std::string& model,
                             RequestParameters* parameters) const {
  auto model_lower = util::toLower(model);
  if (parameters == nullptr) {
    RequestParameters params;
    ::amdinfer::modelLoad(model_lower, &params);
  } else {
    ::amdinfer::modelLoad(model_lower, parameters);
  }
}

std::string NativeClient::workerLoad(const std::string& model,
                                     RequestParameters* parameters) const {
  auto model_lower = util::toLower(model);
  if (parameters == nullptr) {
    RequestParameters params;
    return ::amdinfer::workerLoad(model_lower, &params);
  }
  return ::amdinfer::workerLoad(model_lower, parameters);
}

InferenceResponseFuture NativeClient::modelInferAsync(
  const std::string& workerName, const InferenceRequest& request) const {
#ifdef AMDINFER_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::kCppNative);
#endif

#ifdef AMDINFER_ENABLE_TRACING
  auto trace = startTrace(&(__func__[0]));
  trace->startSpan("C++ enqueue");
#endif
  auto api = std::make_unique<CppNativeApi>(request);
  auto future = api->getPromise()->get_future();
#ifdef AMDINFER_ENABLE_TRACING
  trace->endSpan();
  api->setTrace(std::move(trace));
#endif
  ::amdinfer::modelInfer(workerName, std::move(api));

  return future;
}

InferenceResponse NativeClient::modelInfer(
  const std::string& model, const InferenceRequest& request) const {
  auto future = modelInferAsync(model, request);
  return future.get();
}

void NativeClient::modelUnload(const std::string& model) const {
  auto model_lower = util::toLower(model);
  ::amdinfer::modelUnload(model);
}

void NativeClient::workerUnload(const std::string& model) const {
  auto model_lower = util::toLower(model);
  ::amdinfer::workerUnload(model);
}

bool NativeClient::modelReady(const std::string& model) const {
  try {
    return ::amdinfer::modelReady(model);
  } catch (const invalid_argument&) {
    return false;
  }
}

std::vector<std::string> NativeClient::modelList() const {
  return ::amdinfer::modelList();
}

bool NativeClient::hasHardware(const std::string& name, int num) const {
  return ::amdinfer::hasHardware(name, num);
}

}  // namespace amdinfer
