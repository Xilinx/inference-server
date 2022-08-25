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

#include "proteus/client_api/infer_async.hpp"

#include <queue>
#include <thread>

namespace proteus {

std::vector<InferenceResponse> infer_async_ordered(
  Client* client, const std::string& model,
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
  }
  return responses;
}

std::vector<InferenceResponse> infer_async_ordered_batched(
  Client* client, const std::string& model,
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

}  // namespace proteus
