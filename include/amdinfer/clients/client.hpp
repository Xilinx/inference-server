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

#ifndef GUARD_AMDINFER_CLIENTS_CLIENT
#define GUARD_AMDINFER_CLIENTS_CLIENT

#include <string>
#include <vector>

#include "amdinfer/core/predict_api.hpp"  // for InferenceRequest (ptr only)

namespace amdinfer {

/**
 * @brief The base Client class defines the set of methods that all client
 * implementations must provide. These methods are based on the API defined by
 * KServe, with some extensions. This is a pure virtual class.
 */
class Client {
 public:
  /// Destructor
  virtual ~Client() = default;

  /**
   * @brief Returns the server metadata as a ServerMetadata object
   *
   * @return ServerMetadata
   */
  [[nodiscard]] virtual ServerMetadata serverMetadata() const = 0;
  /**
   * @brief Checks if the server is live
   *
   * @return bool - true if server is live, false otherwise
   */
  [[nodiscard]] virtual bool serverLive() const = 0;
  /**
   * @brief Checks if the server is ready
   *
   * @return bool - true if server is ready, false otherwise
   */
  [[nodiscard]] virtual bool serverReady() const = 0;
  /**
   * @brief Checks if a model/worker is ready
   *
   * @param model name of the model to check
   * @return bool - true if model is ready, false otherwise
   */
  [[nodiscard]] virtual bool modelReady(const std::string& model) const = 0;
  /**
   * @brief Returns the metadata associated with a ready model/worker
   *
   * @param model name of the model/worker to get metadata
   * @return ModelMetadata
   */
  [[nodiscard]] virtual ModelMetadata modelMetadata(
    const std::string& model) const = 0;

  /**
   * @brief Loads a model with the given name and load-time parameters. This
   * method assumes that a directory with this model name already exists in the
   * model repository directory for the server containing the model and its
   * metadata in the right format.
   *
   * @param model name of the model to load from the model repository directory
   * @param parameters load-time parameters for the worker supporting the model
   */
  virtual void modelLoad(const std::string& model,
                         RequestParameters* parameters) const = 0;
  /**
   * @brief Unloads a previously loaded model and shut it down. This is
   * identical in functionality to workerUnload and is provided for symmetry.
   *
   * @param model name of the model to unload
   */
  virtual void modelUnload(const std::string& model) const = 0;

  /**
   * @brief Makes a synchronous inference request to the given model/worker. The
   * contents of the request depends on the model/worker that the request is
   * for.
   *
   * @param model name of the model/worker to request inference to
   * @param request the request
   * @return InferenceResponse
   */
  [[nodiscard]] virtual InferenceResponse modelInfer(
    const std::string& model, const InferenceRequest& request) const = 0;
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
  [[nodiscard]] virtual InferenceResponseFuture modelInferAsync(
    const std::string& model, const InferenceRequest& request) const = 0;
  /**
   * @brief Gets a list of active models on the server, returning their names
   *
   * @return std::vector<std::string>
   */
  [[nodiscard]] virtual std::vector<std::string> modelList() const = 0;

  /**
   * @brief Loads a worker with the given name and load-time parameters.
   *
   * @param worker name of the worker to load
   * @param parameters load-time parameters for the worker
   * @return std::string
   */
  virtual std::string workerLoad(const std::string& worker,
                                 RequestParameters* parameters) const = 0;
  /**
   * @brief Unloads a previously loaded worker and shut it down. This is
   * identical in functionality to modelUnload and is provided for symmetry.
   *
   * @param worker name of the worker to unload
   */
  virtual void workerUnload(const std::string& worker) const = 0;

  /**
   * @brief Checks if the server has the requested number of a specific hardware
   * device
   *
   * @param name name of the hardware device to check
   * @param num number of the device that should exist at minimum
   * @return bool - true if server has at least the requested number of the
   * hardware device, false otherwise
   */
  [[nodiscard]] virtual bool hasHardware(const std::string& name,
                                         int num) const = 0;

 protected:
  Client();
};

/**
 * @brief Checks if the server has a certain extension
 *
 * @param client a pointer to a client object
 * @param extension name of the extension to check on the server
 * @return bool - true if the server has the requested extension
 */
bool serverHasExtension(const Client* client, const std::string& extension);
/**
 * @brief Blocks until the server is ready
 *
 * @param client a pointer to a client object
 */
void waitUntilServerReady(const Client* client);
/**
 * @brief Blocks until the named model/worker is ready
 *
 * @param client a pointer to a client object
 * @param model the model/worker to wait for
 */
void waitUntilModelReady(const Client* client, const std::string& model);

/**
 * @brief Makes inference requests in parallel to the specified model. All
 * requests are sent in parallel and the responses are gathered and returned in
 * the same order.
 *
 * @param client a pointer to a client object
 * @param model the model/worker to make inference requests to
 * @param requests a vector of requests
 * @return std::vector<InferenceResponse>
 */
std::vector<InferenceResponse> inferAsyncOrdered(
  Client* client, const std::string& model,
  const std::vector<InferenceRequest>& requests);
/**
 * @brief Makes inference requests in parallel to the specified model in
 * batches. Each batch of requests are gathered and the responses are added to a
 * vector. Once all the responses are received, the response vector is returned.
 *
 * @param client a pointer to a client object
 * @param model the model/worker to make inference requests to
 * @param requests a vector of requests
 * @param batch_size the number of requests that should be sent in parallel at
 * once
 * @return std::vector<InferenceResponse>
 */
std::vector<InferenceResponse> inferAsyncOrderedBatched(
  Client* client, const std::string& model,
  const std::vector<InferenceRequest>& requests, size_t batch_size);

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CLIENTS_CLIENT
