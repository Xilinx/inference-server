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
 * @brief Defines the methods for interacting with Proteus with WebSocket
 */

#ifndef GUARD_PROTEUS_CLIENTS_WEBSOCKET
#define GUARD_PROTEUS_CLIENTS_WEBSOCKET

#include <memory>  // for unique_ptr
#include <string>  // for string
#include <vector>  // for vector

#include "proteus/clients/client.hpp"    // for Client
#include "proteus/core/predict_api.hpp"  // for InferenceRequest (ptr only)

namespace proteus {

class WebSocketClient : public Client {
 public:
  WebSocketClient() = delete;
  WebSocketClient(const std::string& ws_address,
                  const std::string& http_address);
  ~WebSocketClient();

  ServerMetadata serverMetadata() override;
  bool serverLive() override;
  bool serverReady() override;
  bool modelReady(const std::string& model) override;

  std::string modelLoad(const std::string& model,
                        RequestParameters* parameters) override;
  void modelUnload(const std::string& model) override;
  InferenceResponse modelInfer(const std::string& model,
                               const InferenceRequest& request) override;
  std::vector<std::string> modelList() override;

  void modelInferAsync(const std::string& model,
                       const InferenceRequest& request);
  // TODO(varunsh): change to InferenceReponse
  std::string modelRecv();
  void close();

 private:
  class WebSocketClientImpl;
  std::unique_ptr<WebSocketClientImpl> impl_;
};

}  // namespace proteus

#endif  // GUARD_PROTEUS_CLIENTS_WEBSOCKET
