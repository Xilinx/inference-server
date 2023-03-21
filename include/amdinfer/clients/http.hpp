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
 * @brief Defines the methods for interacting with the server with HTTP/REST
 */

#ifndef GUARD_AMDINFER_CLIENTS_HTTP
#define GUARD_AMDINFER_CLIENTS_HTTP

#include <memory>  // for unique_ptr
#include <string>  // for string
#include <vector>  // for vector

#include "amdinfer/clients/client.hpp"  // IWYU pragma: export
#include "amdinfer/declarations.hpp"    // for StringMap

namespace amdinfer {

class ParameterMap;

/**
 * @brief The HttpClient class implements the Client using HTTP REST
 *
 * @details Usage:
 *
 * HttpClient client{"http://127:0.0.1:8998"};
 * if (client.serverLive()){
 *   ...
 * }
 *
 */
class HttpClient : public Client {
 public:
  /**
   * @brief Construct a new HttpClient object
   *
   * @param address Address of the server to connect to
   */
  explicit HttpClient(const std::string& address);
  /**
   * @brief Construct a new HttpClient object
   *
   * @param address Address of the server to connect to
   * @param headers Key-value pairs that should be added to the HTTP headers for
   * all requests
   * @param parallelism Max number of requests that can be sent in parallel
   */
  HttpClient(const std::string& address, const StringMap& headers,
             int parallelism);

  /// Copy constructor
  HttpClient(HttpClient const&) = delete;
  /// Copy assignment constructor
  HttpClient& operator=(const HttpClient&) = delete;
  /// Move constructor
  HttpClient(HttpClient&& other) = default;
  /// Move assignment constructor
  HttpClient& operator=(HttpClient&& other) = default;
  /**
   * @brief Destructor. This is needed because HttpClientImpl is an incomplete
   * type. The destructor is defaulted in the implementation. But having a non-
   * default destructor here forces the need to explicitly specify the other
   * special member functions by the Rule of 5.
   */
  ~HttpClient() override;

  /**
   * @brief Returns the server metadata as a ServerMetadata object
   *
   * @return ServerMetadata
   */
  [[nodiscard]] ServerMetadata serverMetadata() const override;
  /**
   * @brief Checks if the server is live
   *
   * @return bool - true if server is live, false otherwise
   */
  [[nodiscard]] bool serverLive() const override;
  /**
   * @brief Checks if the server is ready
   *
   * @return bool - true if server is ready, false otherwise
   */
  [[nodiscard]] bool serverReady() const override;
  /**
   * @brief Checks if a model/worker is ready
   *
   * @param model name of the model to check
   * @return bool - true if model is ready, false otherwise
   */
  [[nodiscard]] bool modelReady(const std::string& model) const override;
  /**
   * @brief Returns the metadata associated with a ready model/worker
   *
   * @param model name of the model/worker to get metadata
   * @return ModelMetadata
   */
  [[nodiscard]] ModelMetadata modelMetadata(
    const std::string& model) const override;

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
                 const ParameterMap& parameters) const override;
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
  [[nodiscard]] InferenceResponse modelInfer(
    const std::string& model, const InferenceRequest& request) const override;
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
  [[nodiscard]] InferenceResponseFuture modelInferAsync(
    const std::string& model, const InferenceRequest& request) const override;
  /**
   * @brief Gets a list of active models on the server, returning their names
   *
   * @return std::vector<std::string>
   */
  [[nodiscard]] std::vector<std::string> modelList() const override;

  /**
   * @brief Loads a worker with the given name and load-time parameters.
   *
   * @param worker name of the worker to load
   * @param parameters load-time parameters for the worker
   * @return std::string
   */
  [[nodiscard]] std::string workerLoad(
    const std::string& worker, const ParameterMap& parameters) const override;
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
  [[nodiscard]] bool hasHardware(const std::string& name,
                                 int num) const override;

 private:
  class HttpClientImpl;
  std::unique_ptr<HttpClientImpl> impl_;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CLIENTS_HTTP
