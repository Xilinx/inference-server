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

#include "amdinfer/clients/client.hpp"

#include <algorithm>      // for copy, copy_backward
#include <chrono>         // for seconds
#include <future>         // for future
#include <queue>          // for queue
#include <thread>         // for sleep_for
#include <unordered_set>  // for operator!=, unordered_set

#include "amdinfer/build_options.hpp"            // for AMDINFER_ENABLE_LOGGING
#include "amdinfer/core/exceptions.hpp"          // for connection_error
#include "amdinfer/core/inference_request.hpp"   // for InferenceRequest
#include "amdinfer/core/inference_response.hpp"  // for InferenceResponse
#include "amdinfer/observation/logging.hpp"  // for getLogDirectory, initLogger

namespace amdinfer {

void initializeClientLogging() {
#ifdef AMDINFER_ENABLE_LOGGING
  LogOptions options{
    "client",  // logger_name
    getLogDirectory(),
    true,             // enable file logging
    LogLevel::Debug,  // file log level
    true,             // enable console logging
    LogLevel::Warn    // console log level
  };
  initLogger(options);
#endif
}

std::vector<std::string> loadEnsemble(const Client* client,
                                      std::vector<std::string> workers,
                                      std::vector<ParameterMap> parameters) {
  if (workers.size() != parameters.size()) {
    throw invalid_argument("The number of workers and parameters must match");
  }

  std::vector<std::string> endpoints;
  endpoints.resize(workers.size());

  std::string next;
  // reverse iterate through vectors using "goes to" operator
  for (auto i = workers.size(); i-- > 0;) {
    const auto& worker = workers.at(i);
    auto& parameter = parameters.at(i);

    if (!next.empty()) {
      parameter.put("next", next);
    }
    auto endpoint = client->workerLoad(worker, parameter);
    next = endpoint;
    endpoints.at(i) = std::move(endpoint);
    waitUntilModelReady(client, next);
  }

  return endpoints;
}

void unloadModels(const Client* client,
                  const std::vector<std::string>& models) {
  for (const auto& model : models) {
    client->modelUnload(model);
  }
}

Client::Client() { initializeClientLogging(); }

bool serverHasExtension(const Client* client, const std::string& extension) {
  auto metadata = client->serverMetadata();
  return metadata.extensions.find(extension) != metadata.extensions.end();
}

void waitUntilServerReady(const Client* client) {
  bool ready = false;
  while (!ready) {
    try {
      ready = client->serverReady();
    } catch (const amdinfer::connection_error&) {
      // ignore connection errors
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}

void waitUntilModelReady(const Client* client, const std::string& model) {
  bool ready = false;
  // arbitrarily set to 1ms
  const auto sleep_time = std::chrono::milliseconds(1);
  while (!ready) {
    ready = client->modelReady(model);
    std::this_thread::sleep_for(sleep_time);
  }
}

void waitUntilModelNotReady(const Client* client, const std::string& model) {
  bool ready = true;
  // arbitrarily set to 1ms
  const auto sleep_time = std::chrono::milliseconds(1);
  while (ready) {
    ready = client->modelReady(model);
    std::this_thread::sleep_for(sleep_time);
  }
}

std::vector<InferenceResponse> inferAsyncOrdered(
  const Client* client, const std::string& model,
  const std::vector<InferenceRequest>& requests) {
  std::queue<InferenceResponseFuture> q;
  for (const auto& request : requests) {
    q.push(client->modelInferAsync(model, request));
  }

  const auto num_requests = requests.size();
  std::vector<InferenceResponse> responses;
  responses.reserve(num_requests);
  for (auto i = 0U; i < num_requests; ++i) {
    auto& future = q.front();
    responses.push_back(future.get());
    q.pop();
  }
  return responses;
}

std::vector<InferenceResponse> inferAsyncOrderedBatched(
  const Client* client, const std::string& model,
  const std::vector<InferenceRequest>& requests, size_t batch_size) {
  auto num_requests = requests.size();
  std::vector<InferenceResponse> responses;
  responses.reserve(num_requests);
  auto start_index = 0U;
  std::queue<InferenceResponseFuture> q;

  while (start_index + batch_size < num_requests) {
    for (auto i = start_index; i < batch_size; ++i) {
      q.push(client->modelInferAsync(model, requests[i]));
    }

    for (auto i = 0U; i < batch_size; ++i) {
      auto& future = q.front();
      responses.push_back(future.get());
      q.pop();
    }
    start_index += batch_size;
  }

  if (start_index != num_requests) {
    for (auto i = start_index; i < num_requests; ++i) {
      q.push(client->modelInferAsync(model, requests[i]));
    }

    for (auto i = 0U; i < num_requests - start_index; ++i) {
      auto& future = q.front();
      responses.push_back(future.get());
      q.pop();
    }
  }
  return responses;
}

}  // namespace amdinfer
