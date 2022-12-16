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

/**
 * @brief The WebSocketClient class implements the Client using websocket. It
 * reuses the HttpClient for most transactions with the exception of some
 * operations that actually use websocket.
 *
 * @details Usage:
 *
 * WebSocketClient client{"ws://127.0.0.1:8998", "http://127.0.0.1:8998"};
 * if (client.serverLive()){
 *   ...
 * }
 *
 */
class WebSocketClient : public Client {
 public:
  /**
   * @brief Constructs a new WebSocketClient object
   *
   * @param ws_address address of the websocket server to connect to
   * @param http_address address of the HTTP server to connect to
   */
  WebSocketClient(const std::string& ws_address,
                  const std::string& http_address);
  /// Destructor
  ~WebSocketClient() override;

  /**
   * @brief Returns the server metadata as a ServerMetadata object
   *
   * @return ServerMetadata
   */
  ServerMetadata serverMetadata() const override;
  /**
   * @brief Checks if the server is live
   *
   * @return bool - true if server is live, false otherwise
   */
  bool serverLive() const override;
  /**
   * @brief Checks if the server is ready
   *
   * @return bool - true if server is ready, false otherwise
   */
  bool serverReady() const override;
  /**
   * @brief Checks if a model/worker is ready
   *
   * @param model name of the model to check
   * @return bool - true if model is ready, false otherwise
   */
  bool modelReady(const std::string& model) const override;
  /**
   * @brief Returns the metadata associated with a ready model/worker
   *
   * @param model name of the model/worker to get metadata
   * @return ModelMetadata
   */
  ModelMetadata modelMetadata(const std::string& model) const override;

  /**
   * @brief Loads a model with the given name and load-time parameters. This
   * method assumes that a directory with this model name already exists in the
   * model repository directory for the server containing the model and its
   * metadata in the right format.
   *
   * @param model name of the model to load from the model repository directory
   * @param parameters load-time parameters for the worker supporting the model
   */
  void modelLoad(const std::string& model,
                 RequestParameters* parameters) const override;
  /**
   * @brief Unloads a previously loaded model and shut it down. This is
   * identical in functionality to workerUnload and is provided for symmetry.
   *
   * @param model name of the model to unload
   */
  void modelUnload(const std::string& model) const override;

  /**
   * @brief Makes a synchronous inference request to the given model/worker. The
   * contents of the request depends on the model/worker that the request is
   * for.
   *
   * @param model name of the model/worker to request inference to
   * @param request the request
   * @return InferenceResponse
   */
  InferenceResponse modelInfer(const std::string& model,
                               const InferenceRequest& request) const override;
  /**
   * @brief Makes an asynchronous inference request to the given model/worker.
   * The contents of the request depends on the model/worker that the request
   * is for. The user must save the Future object and use it to get the results
   * of the inference later.
   *
   * @param model name of the model/worker to request inference to
   * @param request the request
   * @return InferenceResponseFuture
   */
  InferenceResponseFuture modelInferAsync(
    const std::string& model, const InferenceRequest& request) const override;
  /**
   * @brief Gets a list of active models on the server, returning their names
   *
   * @return std::vector<std::string>
   */
  std::vector<std::string> modelList() const override;

  /**
   * @brief Loads a worker with the given name and load-time parameters.
   *
   * @param worker name of the worker to load
   * @param parameters load-time parameters for the worker
   * @return std::string
   */
  std::string workerLoad(const std::string& worker,
                         RequestParameters* parameters) const override;
  /**
   * @brief Unloads a previously loaded worker and shut it down. This is
   * identical in functionality to modelUnload and is provided for symmetry.
   *
   * @param worker name of the worker to unload
   */
  void workerUnload(const std::string& worker) const override;

  /**
   * @brief Checks if the server has the requested number of a specific hardware
   * device
   *
   * @param name name of the hardware device to check
   * @param num number of the device that should exist at minimum
   * @return bool - true if server has at least the requested number of the
   * hardware device, false otherwise
   */
  bool hasHardware(const std::string& name, int num) const override;

  /**
   * @brief Makes a websocket inference request to the given model/worker. The
   * contents of the request depends on the model/worker that the request is
   * for. This method differs from the standard inference in that it submits an
   * actual Websocket message. The user should use modelRecv to get results and
   * must disambiguate different responses on the client-side using the IDs of
   * the responses.
   *
   * @param model
   * @param request
   */
  void modelInferWs(const std::string& model,
                    const InferenceRequest& request) const;
  // TODO(varunsh) const: change to InferenceResponse
  /**
   * @brief Gets one message from the websocket server sent in response to a
   * modelInferWs request. The user should know beforehand how many messages are
   * expected and should call this method the same number of times.
   *
   * @return std::string a JSON object encoded as a string
   */
  std::string modelRecv() const;
  /**
   * @brief Closes the websocket connection
   *
   */
  void close() const;

 private:
  class WebSocketClientImpl;
  std::unique_ptr<WebSocketClientImpl> impl_;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CLIENTS_WEBSOCKET
