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
 * @brief Defines the base class for all clients
 */

#ifndef GUARD_PROTEUS_CLIENTS_CLIENT
#define GUARD_PROTEUS_CLIENTS_CLIENT

#include <string>
#include <vector>

#include "proteus/core/predict_api.hpp"  // for InferenceRequest (ptr only)

namespace proteus {

class Client {
 public:
  virtual ~Client() = default;

  virtual ServerMetadata serverMetadata() const = 0;
  virtual bool serverLive() const = 0;
  virtual bool serverReady() const = 0;
  virtual bool modelReady(const std::string& model) const = 0;
  virtual ModelMetadata modelMetadata(const std::string& model) const = 0;

  virtual void modelLoad(const std::string& model,
                         RequestParameters* parameters) const = 0;
  virtual void modelUnload(const std::string& model) const = 0;

  virtual InferenceResponse modelInfer(
    const std::string& model, const InferenceRequest& request) const = 0;
  virtual InferenceResponseFuture modelInferAsync(
    const std::string& model, const InferenceRequest& request) const = 0;
  virtual std::vector<std::string> modelList() const = 0;

  virtual std::string workerLoad(const std::string& worker,
                                 RequestParameters* parameters) const = 0;
  virtual void workerUnload(const std::string& worker) const = 0;

  virtual bool hasHardware(const std::string& name, int num) const = 0;

 protected:
  Client();
};

bool serverHasExtension(const Client* client, const std::string& extension);
void waitUntilServerReady(const Client* client);
void waitUntilModelReady(const Client* client, const std::string& model);

std::vector<InferenceResponse> inferAsyncOrdered(
  Client* client, const std::string& model,
  const std::vector<InferenceRequest>& requests);
std::vector<InferenceResponse> inferAsyncOrderedBatched(
  Client* client, const std::string& model,
  const std::vector<InferenceRequest>& requests, size_t batch_size);

}  // namespace proteus

#endif  // GUARD_PROTEUS_CLIENTS_CLIENT
