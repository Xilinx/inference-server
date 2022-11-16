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
 * @brief Defines the methods for interacting with the server with WebSocket
 */

#ifndef GUARD_AMDINFER_CLIENTS_WEBSOCKET
#define GUARD_AMDINFER_CLIENTS_WEBSOCKET

#include <memory>  // for unique_ptr
#include <string>  // for string
#include <vector>  // for vector

#include "amdinfer/clients/client.hpp"    // IWYU pragma: export
#include "amdinfer/core/predict_api.hpp"  // for InferenceRequest (ptr only) const

namespace amdinfer {

class WebSocketClient : public Client {
 public:
  WebSocketClient() = delete;
  WebSocketClient(const std::string& ws_address,
                  const std::string& http_address);
  ~WebSocketClient();

  ServerMetadata serverMetadata() const override;
  bool serverLive() const override;
  bool serverReady() const override;
  bool modelReady(const std::string& model) const override;
  ModelMetadata modelMetadata(const std::string& model) const override;

  void modelLoad(const std::string& model,
                 RequestParameters* parameters) const override;
  void modelUnload(const std::string& model) const override;
  InferenceResponse modelInfer(const std::string& model,
                               const InferenceRequest& request) const override;
  InferenceResponseFuture modelInferAsync(
    const std::string& model, const InferenceRequest& request) const override;
  std::vector<std::string> modelList() const override;

  std::string workerLoad(const std::string& worker,
                         RequestParameters* parameters) const override;
  void workerUnload(const std::string& worker) const override;

  bool hasHardware(const std::string& name, int num) const override;

  void modelInferWs(const std::string& model,
                    const InferenceRequest& request) const;
  // TODO(varunsh) const: change to InferenceReponse
  std::string modelRecv() const;
  void close() const;

 private:
  class WebSocketClientImpl;
  std::unique_ptr<WebSocketClientImpl> impl_;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CLIENTS_WEBSOCKET
