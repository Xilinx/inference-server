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

/**
 * @file
 * @brief Defines the methods for interacting with Proteus with HTTP/REST
 */

#ifndef GUARD_PROTEUS_CLIENTS_HTTP
#define GUARD_PROTEUS_CLIENTS_HTTP

#include <memory>         // for unique_ptr
#include <string>         // for string
#include <unordered_map>  // for unordered_map
#include <vector>         // for vector

#include "proteus/clients/client.hpp"    // for Client
#include "proteus/core/predict_api.hpp"  // for InferenceRequest (ptr only)

namespace proteus {

// /**
//  * @brief Start the HTTP server for collecting metrics. This is a no-op if
//  * Proteus is compiled without HTTP support.
//  *
//  * @param port port to use
//  */
// void startHttpServer(int port);

/**
 * @brief Stop the HTTP server. This is a no-op if Proteus is compiled without
 * HTTP support.
 *
 */
// void stopHttpServer();

class HttpClient : public Client {
 public:
  HttpClient() = delete;
  HttpClient(const std::string& address, const StringMap& headers = {});
  ~HttpClient();

  ServerMetadata serverMetadata() override;
  bool serverLive() override;
  bool serverReady() override;
  bool modelReady(const std::string& model) override;
  ModelMetadata modelMetadata(const std::string& model) override;

  void modelLoad(const std::string& model,
                 RequestParameters* parameters) override;
  void modelUnload(const std::string& model) override;
  InferenceResponse modelInfer(const std::string& model,
                               const InferenceRequest& request) override;
  std::vector<std::string> modelList() override;

  std::string workerLoad(const std::string& worker,
                         RequestParameters* parameters) override;
  void workerUnload(const std::string& worker) override;

 private:
  class HttpClientImpl;
  std::unique_ptr<HttpClientImpl> impl_;
};

}  // namespace proteus

#endif  // GUARD_PROTEUS_CLIENTS_HTTP
