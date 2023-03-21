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

#include "amdinfer/buffers/buffer.hpp"           // for BufferPtr
#include "amdinfer/build_options.hpp"            // for AMDINFER_ENABLE_TRACING
#include "amdinfer/core/data_types.hpp"          // for DataType
#include "amdinfer/core/exceptions.hpp"          // for invalid_argument
#include "amdinfer/core/inference_request.hpp"   // for InferenceRequest
#include "amdinfer/core/inference_response.hpp"  // for InferenceResponse
#include "amdinfer/core/parameters.hpp"          // for ParameterMap
#include "amdinfer/core/request_container.hpp"   // for RequestContainer
#include "amdinfer/core/shared_state.hpp"        // for SharedState
#include "amdinfer/observation/metrics.hpp"      // for Metrics, MetricCount...
#include "amdinfer/observation/tracing.hpp"      // for startTrace, Trace
#include "amdinfer/servers/server.hpp"           // for Server
#include "amdinfer/servers/server_internal.hpp"  // for Server::ServerImpl
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
  return SharedState::serverMetadata();
}
bool NativeClient::serverLive() const { return true; }
bool NativeClient::serverReady() const { return true; }

ModelMetadata NativeClient::modelMetadata(const std::string& model) const {
  return impl_->state->modelMetadata(model);
}

void NativeClient::modelLoad(const std::string& model,
                             const ParameterMap& parameters) const {
  auto model_lower = util::toLower(model);
  impl_->state->modelLoad(model_lower, parameters);
}

std::string NativeClient::workerLoad(const std::string& worker,
                                     const ParameterMap& parameters) const {
  auto worker_lower = util::toLower(worker);
  return impl_->state->workerLoad(worker_lower, parameters);
}

InferenceRequestPtr getRequest(const InferenceRequest& req,
                               const MemoryPool* pool) {
  auto request = std::make_shared<InferenceRequest>(req);

  const auto& inputs = request->getInputs();
  int i = 0;
  for (const auto& input : inputs) {
    auto size = input.getSize() * input.getDatatype().size();
    auto buffer = pool->get({MemoryAllocators::Cpu}, input, 1);
    buffer->write(input.getData(), 0, size);
    request->setInputTensorData(i, buffer->data(0));
    i++;
  }

  return request;
}

InferenceResponseFuture setCallback(InferenceRequest* request) {
  auto promise = std::make_shared<std::promise<amdinfer::InferenceResponse>>();
  auto future = promise->get_future();
  Callback callback = [promise](const InferenceResponse& response) {
    promise->set_value(response);
  };
  request->setCallback(std::move(callback));
  return future;
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
  auto new_request = getRequest(request, impl_->state->getPool());
  auto future = setCallback(new_request.get());
  auto request_container = std::make_unique<RequestContainer>();
  request_container->request = std::move(new_request);

#ifdef AMDINFER_ENABLE_TRACING
  trace->endSpan();
  request_container->trace = std::move(trace);
#endif
  impl_->state->modelInfer(model, std::move(request_container));

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
  return SharedState::hasHardware(name, num);
}

}  // namespace amdinfer
