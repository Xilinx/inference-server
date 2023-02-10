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
#include "amdinfer/core/exceptions.hpp"          // for invalid_argument
#include "amdinfer/core/interface.hpp"           // for Interface
#include "amdinfer/observation/metrics.hpp"      // for Metrics, MetricCounte...
#include "amdinfer/observation/tracing.hpp"      // for startTrace, Trace
#include "amdinfer/servers/server_internal.hpp"  // for Server, ServerImpl
#include "amdinfer/util/string.hpp"              // for toLower

namespace amdinfer {

struct NativeClient::NativeClientImpl {
  SharedState* state;
};

NativeClient::NativeClient(Server* server) {
  impl_ = std::make_unique<NativeClientImpl>();
  impl_->state = &(server->impl_->state);
}

NativeClient::~NativeClient() = default;

ServerMetadata NativeClient::serverMetadata() const {
  return impl_->state->serverMetadata();
}
bool NativeClient::serverLive() const { return true; }
bool NativeClient::serverReady() const { return true; }

ModelMetadata NativeClient::modelMetadata(const std::string& model) const {
  return impl_->state->modelMetadata(model);
}

void NativeClient::modelLoad(const std::string& model,
                             ParameterMap* parameters) const {
  auto model_lower = util::toLower(model);
  if (parameters == nullptr) {
    ParameterMap params;
    impl_->state->modelLoad(model_lower, &params);
  } else {
    impl_->state->modelLoad(model_lower, parameters);
  }
}

std::string NativeClient::workerLoad(const std::string& worker,
                                     ParameterMap* parameters) const {
  auto worker_lower = util::toLower(worker);
  if (parameters == nullptr) {
    ParameterMap params;
    return impl_->state->workerLoad(worker_lower, &params);
  }
  return impl_->state->workerLoad(worker_lower, parameters);
}

InferenceResponseFuture NativeClient::modelInferAsync(
  const std::string& model, const InferenceRequest& request) const {
#ifdef AMDINFER_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::CppNative);
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
  impl_->state->modelInfer(model, std::move(api));

  return future;
}

InferenceResponse NativeClient::modelInfer(
  const std::string& model, const InferenceRequest& request) const {
  auto future = modelInferAsync(model, request);
  return future.get();
}

void NativeClient::modelUnload(const std::string& model) const {
  auto model_lower = util::toLower(model);
  impl_->state->modelUnload(model_lower);
}

void NativeClient::workerUnload(const std::string& worker) const {
  auto worker_lower = util::toLower(worker);
  impl_->state->workerUnload(worker_lower);
}

bool NativeClient::modelReady(const std::string& model) const {
  try {
    return impl_->state->modelReady(model);
  } catch (const invalid_argument&) {
    return false;
  }
}

std::vector<std::string> NativeClient::modelList() const {
  return impl_->state->modelList();
}

bool NativeClient::hasHardware(const std::string& name, int num) const {
  return impl_->state->hasHardware(name, num);
}

}  // namespace amdinfer
