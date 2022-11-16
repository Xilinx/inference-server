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
 * @brief Defines the methods for interacting with Proteus with gRPC
 */

#ifndef GUARD_PROTEUS_CLIENTS_GRPC
#define GUARD_PROTEUS_CLIENTS_GRPC

#include <memory>  // for shared_ptr, unique_ptr
#include <string>  // for string
#include <vector>  // for vector

#include "amdinfer/clients/client.hpp"    // IWYU pragma: export
#include "amdinfer/core/predict_api.hpp"  // for InferenceRequest (ptr only) const

namespace grpc {
class Channel;
}

namespace amdinfer {

class GrpcClient : public Client {
 public:
  explicit GrpcClient(const std::string& address);
  explicit GrpcClient(const std::shared_ptr<::grpc::Channel>& channel);
  ~GrpcClient() override;

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
  // int streamModelInferStart(const std::string& model,
  //                           const InferenceRequest& request,
  //                           RequestParameters& metadata) const;
  // bool streamModelInfer(int index, InferenceResponse& response) const;

  std::string workerLoad(const std::string& worker,
                         RequestParameters* parameters) const override;
  void workerUnload(const std::string& worker) const override;

  bool hasHardware(const std::string& name, int num) const override;

 private:
  class GrpcClientImpl;
  std::unique_ptr<GrpcClientImpl> impl_;
};

}  // namespace amdinfer

#endif  // GUARD_PROTEUS_CLIENTS_GRPC
