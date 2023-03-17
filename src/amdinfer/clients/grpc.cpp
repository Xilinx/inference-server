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
 * @brief Implements the methods for interacting with the server with gRPC
 */

#include "amdinfer/clients/grpc.hpp"

#include <google/protobuf/repeated_ptr_field.h>  // for RepeatedPtrField
#include <grpcpp/grpcpp.h>                       // for Status, ClientContext

#include <future>         // for __forced_unwind, async
#include <memory>         // for unique_ptr, shared_ptr
#include <string>         // for string
#include <unordered_set>  // for unordered_set
#include <vector>         // for vector

#include "amdinfer/clients/grpc_internal.hpp"    // for mapParametersToProto
#include "amdinfer/core/data_types.hpp"          // for DataType
#include "amdinfer/core/exceptions.hpp"          // for bad_status, connecti...
#include "amdinfer/core/inference_request.hpp"   // for InferenceRequest
#include "amdinfer/core/inference_response.hpp"  // for InferenceResponse
#include "amdinfer/core/parameters.hpp"          // for ParameterMap
#include "amdinfer/declarations.hpp"             // for InferenceResponseFuture
#include "amdinfer/observation/observer.hpp"     // for Logger, Observer
#include "inference.grpc.pb.h"                   // for GRPCInferenceService...
#include "inference.pb.h"                        // for ModelMetadataRespons...

using grpc::ClientContext;
using grpc::Status;

namespace amdinfer {

class GrpcClient::GrpcClientImpl {
 public:
  explicit GrpcClientImpl(const std::shared_ptr<::grpc::Channel>& channel) {
    this->stub_ = inference::GRPCInferenceService::NewStub(channel);
  }

  inference::GRPCInferenceService::Stub* getStub() { return this->stub_.get(); }

 private:
  std::unique_ptr<inference::GRPCInferenceService::Stub> stub_;
};

GrpcClient::GrpcClient(const std::string& address)
  : GrpcClient(
      ::grpc::CreateChannel(address, ::grpc::InsecureChannelCredentials())) {}

GrpcClient::GrpcClient(const std::shared_ptr<::grpc::Channel>& channel) {
  this->impl_ = std::make_unique<GrpcClient::GrpcClientImpl>(channel);
}

GrpcClient::~GrpcClient() = default;

ServerMetadata GrpcClient::serverMetadata() const {
  inference::ServerMetadataRequest request;
  inference::ServerMetadataResponse reply;

  ClientContext context;

  auto* stub = this->impl_->getStub();
  Status status = stub->ServerMetadata(&context, request, &reply);

  if (status.ok()) {
    auto ext = reply.extensions();
    std::unordered_set<std::string> extensions(ext.begin(), ext.end());
    ServerMetadata metadata{reply.name(), reply.version(), extensions};
    return metadata;
  }
  throw bad_status(status.error_message());
}

bool GrpcClient::serverLive() const {
  inference::ServerLiveRequest request;
  inference::ServerLiveResponse reply;

  ClientContext context;

  auto* stub = this->impl_->getStub();
  Status status = stub->ServerLive(&context, request, &reply);

  if (status.ok()) {
    return reply.live();
  }
  if (status.error_code() == ::grpc::StatusCode::UNAVAILABLE) {
    return false;
  }
  throw bad_status(status.error_message());
}

bool GrpcClient::serverReady() const {
  inference::ServerReadyRequest request;
  inference::ServerReadyResponse reply;

  ClientContext context;

  auto* stub = this->impl_->getStub();
  Status status = stub->ServerReady(&context, request, &reply);

  if (status.ok()) {
    return reply.ready();
  }
  if (status.error_code() == ::grpc::StatusCode::UNAVAILABLE) {
    throw connection_error(status.error_message());
  }
  throw bad_status(status.error_message());
}

bool GrpcClient::modelReady(const std::string& model) const {
  inference::ModelReadyRequest request;
  inference::ModelReadyResponse reply;

  ClientContext context;

  request.set_name(model);

  auto* stub = this->impl_->getStub();
  Status status = stub->ModelReady(&context, request, &reply);

  if (status.ok()) {
    return reply.ready();
  }
  return false;
}

ModelMetadata mapProtoToModelMetadata(
  const inference::ModelMetadataResponse& resp) {
  ModelMetadata metadata{resp.name(), resp.platform()};
  const auto& inputs = resp.inputs();
  for (const auto& input : inputs) {
    std::vector<int> shape;
    shape.reserve(input.shape_size());
    for (const auto& index : input.shape()) {
      shape.push_back(static_cast<int>(index));
    }
    metadata.addInputTensor(input.name(), DataType(input.datatype().c_str()),
                            shape);
  }
  const auto& outputs = resp.outputs();
  for (const auto& output : outputs) {
    std::vector<int> shape;
    shape.reserve(output.shape_size());
    for (const auto& index : output.shape()) {
      shape.push_back(static_cast<int>(index));
    }
    metadata.addInputTensor(output.name(), DataType(output.datatype().c_str()),
                            shape);
  }
  return metadata;
}

ModelMetadata GrpcClient::modelMetadata(const std::string& model) const {
  inference::ModelMetadataRequest request;
  inference::ModelMetadataResponse reply;

  ClientContext context;

  request.set_name(model);

  auto* stub = this->impl_->getStub();
  Status status = stub->ModelMetadata(&context, request, &reply);

  if (status.ok()) {
    return mapProtoToModelMetadata(reply);
  }
  throw bad_status(status.error_message());
}

std::vector<std::string> GrpcClient::modelList() const {
  inference::ModelListRequest request;
  inference::ModelListResponse reply;

  ClientContext context;

  auto* stub = this->impl_->getStub();
  Status status = stub->ModelList(&context, request, &reply);

  if (status.ok()) {
    auto mods = reply.models();
    std::vector<std::string> models(mods.begin(), mods.end());
    return models;
  }
  throw bad_status(status.error_message());
}

void GrpcClient::modelLoad(const std::string& model,
                           const ParameterMap& parameters) const {
  inference::ModelLoadRequest request;
  inference::ModelLoadResponse reply;

  ClientContext context;

  request.set_name(model);
  auto* params = request.mutable_parameters();
  mapParametersToProto(parameters.data(), params);

  auto* stub = this->impl_->getStub();
  Status status = stub->ModelLoad(&context, request, &reply);

  if (!status.ok()) {
    throw bad_status(status.error_message());
  }
}

void GrpcClient::modelUnload(const std::string& model) const {
  inference::ModelUnloadRequest request;
  inference::ModelUnloadResponse reply;

  ClientContext context;

  request.set_name(model);

  auto* stub = this->impl_->getStub();
  Status status = stub->ModelUnload(&context, request, &reply);

  if (!status.ok()) {
    throw bad_status(status.error_message());
  }
}

std::string GrpcClient::workerLoad(const std::string& worker,
                                   const ParameterMap& parameters) const {
  inference::WorkerLoadRequest request;
  inference::WorkerLoadResponse reply;

  ClientContext context;

  request.set_name(worker);
  auto* params = request.mutable_parameters();
  mapParametersToProto(parameters.data(), params);

  auto* stub = this->impl_->getStub();
  Status status = stub->WorkerLoad(&context, request, &reply);

  if (status.ok()) {
    return reply.endpoint();
  }
  throw bad_status(status.error_message());
}

void GrpcClient::workerUnload(const std::string& worker) const {
  inference::WorkerUnloadRequest request;
  inference::WorkerUnloadResponse reply;

  ClientContext context;

  request.set_name(worker);

  auto* stub = this->impl_->getStub();
  Status status = stub->WorkerUnload(&context, request, &reply);

  if (!status.ok()) {
    throw bad_status(status.error_message());
  }
}

InferenceResponse runInference(inference::GRPCInferenceService::Stub* stub,
                               const std::string& model,
                               const InferenceRequest& request) {
  inference::ModelInferRequest grpc_request;
  inference::ModelInferResponse reply;

  ClientContext context;

  Observer observer;
  AMDINFER_IF_LOGGING(observer.logger = Logger{Loggers::Client});

  grpc_request.set_model_name(model);
  mapRequestToProto(request, grpc_request, observer);

  Status status = stub->ModelInfer(&context, grpc_request, &reply);

  if (!status.ok()) {
    throw bad_status(status.error_message());
  }

  InferenceResponse response;
  mapProtoToResponse(reply, response, observer);
  return response;
}

InferenceResponseFuture GrpcClient::modelInferAsync(
  const std::string& model, const InferenceRequest& request) const {
  return std::async(runInference, this->impl_->getStub(), model, request);
}

InferenceResponse GrpcClient::modelInfer(
  const std::string& model, const InferenceRequest& request) const {
  return runInference(this->impl_->getStub(), model, request);
}

bool GrpcClient::hasHardware(const std::string& name, int num) const {
  inference::HasHardwareRequest grpc_request;
  inference::HasHardwareResponse reply;

  ClientContext context;

  grpc_request.set_name(name);
  grpc_request.set_num(num);

  auto* stub = this->impl_->getStub();
  Status status = stub->HasHardware(&context, grpc_request, &reply);

  if (!status.ok()) {
    throw bad_status(status.error_message());
  }

  return reply.found();
}

}  // namespace amdinfer
