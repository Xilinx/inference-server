// Copyright 2022 Xilinx, Inc.
// Copyright 2022 Advanced Micro Devices, Inc.
// Copyright 2022 Advanced Micro Devices Inc.
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

#include "amdinfer/clients/client.hpp"    // IWYU pragma: export
#include "amdinfer/core/predict_api.hpp"  // for RequestParameters (ptr o...
#include "amdinfer/declarations.hpp"      // for StringMap

namespace amdinfer {

class HttpClient : public Client {
 public:
  HttpClient(std::string address, const StringMap& headers = {},
             int parallelism = 32);
  HttpClient(HttpClient const&);
  HttpClient& operator=(const HttpClient&) const = delete;
  HttpClient(HttpClient&& other) noexcept;
  HttpClient& operator=(HttpClient&& other) const noexcept = delete;
  ~HttpClient() override;

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

  const std::string& getAddress() const&;
  std::string getAddress() const&&;
  const StringMap& getHeaders() const&;
  StringMap getHeaders() const&&;

 private:
  std::string address_;
  StringMap headers_;

  class HttpClientImpl;
  std::unique_ptr<HttpClientImpl> impl_;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CLIENTS_HTTP