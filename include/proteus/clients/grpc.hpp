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

#include "proteus/clients/client.hpp"    // for Client
#include "proteus/core/predict_api.hpp"  // for InferenceRequest (ptr only)

namespace grpc {
class Channel;
}

namespace proteus {

/**
 * @brief Start the gRPC server. This is a no-op if Proteus is compiled without
 * gRPC support.
 *
 * @param port port to use
 */
void startGrpcServer(int port);

void stopGrpcServer();

class GrpcClient : public Client {
 public:
  GrpcClient() = delete;
  GrpcClient(const std::string& address);
  GrpcClient(const std::shared_ptr<::grpc::Channel>& channel);
  ~GrpcClient();

  ServerMetadata serverMetadata() override;
  bool serverLive() override;
  bool serverReady() override;
  bool modelReady(const std::string& model) override;

  void modelLoad(const std::string& model,
                 RequestParameters* parameters) override;
  void modelUnload(const std::string& model) override;
  InferenceResponse modelInfer(const std::string& model,
                               const InferenceRequest& request) override;
  std::vector<std::string> modelList() override;
  // int streamModelInferStart(const std::string& model,
  //                           const InferenceRequest& request,
  //                           RequestParameters& metadata);
  // bool streamModelInfer(int index, InferenceResponse& response);

  std::string workerLoad(const std::string& worker,
                         RequestParameters* parameters) override;
  void workerUnload(const std::string& worker) override;

 private:
  class GrpcClientImpl;
  std::unique_ptr<GrpcClientImpl> impl_;
};

}  // namespace proteus

#endif  // GUARD_PROTEUS_CLIENTS_GRPC
