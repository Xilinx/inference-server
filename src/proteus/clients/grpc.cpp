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
 * @brief Implements the methods for interacting with Proteus with gRPC
 */

#include "proteus/clients/grpc.hpp"

#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>

#include "predict_api.grpc.pb.h"
#include "proteus/build_options.hpp"
#include "proteus/core/data_types.hpp"
#include "proteus/servers/grpc_server.hpp"

using grpc::ClientContext;
using grpc::Status;

namespace proteus {

class GrpcClient::GrpcClientImpl {
 public:
  GrpcClientImpl(std::shared_ptr<::grpc::Channel> channel) {
    this->stub_ = inference::GRPCInferenceService::NewStub(channel);
  }

  inference::GRPCInferenceService::Stub* getStub() { return this->stub_.get(); }

 private:
  std::unique_ptr<inference::GRPCInferenceService::Stub> stub_;
};

GrpcClient::GrpcClient(const std::string& address)
  : GrpcClient(
      ::grpc::CreateChannel(address, ::grpc::InsecureChannelCredentials())) {}

GrpcClient::GrpcClient(std::shared_ptr<::grpc::Channel> channel) {
  this->impl_ = std::make_unique<GrpcClient::GrpcClientImpl>(channel);
}

GrpcClient::~GrpcClient() = default;

bool GrpcClient::serverLive() {
  inference::ServerLiveRequest request;
  inference::ServerLiveResponse reply;

  ClientContext context;

  auto* stub = this->impl_->getStub();
  Status status = stub->ServerLive(&context, request, &reply);

  if (status.ok()) {
    return reply.live();
  } else {
    throw std::runtime_error(status.error_message());
  }
}

bool GrpcClient::serverReady() {
  inference::ServerReadyRequest request;
  inference::ServerReadyResponse reply;

  ClientContext context;

  auto* stub = this->impl_->getStub();
  Status status = stub->ServerReady(&context, request, &reply);

  if (status.ok()) {
    return reply.ready();
  } else {
    throw std::runtime_error(status.error_message());
  }
}

bool GrpcClient::modelReady(const std::string& model) {
  inference::ModelReadyRequest request;
  inference::ModelReadyResponse reply;

  ClientContext context;

  request.set_name(model);

  auto* stub = this->impl_->getStub();
  Status status = stub->ModelReady(&context, request, &reply);

  if (status.ok()) {
    return reply.ready();
  } else {
    throw std::runtime_error(status.error_message());
  }
}

std::string GrpcClient::modelLoad(const std::string& model) {
  inference::ModelLoadRequest request;
  inference::ModelLoadResponse reply;

  ClientContext context;

  request.set_name(model);

  auto* stub = this->impl_->getStub();
  Status status = stub->ModelLoad(&context, request, &reply);

  if (status.ok()) {
    return reply.endpoint();
  } else {
    throw std::runtime_error(status.error_message());
  }
}

void setParameters(
  const std::map<std::string, proteus::Parameter>& parameters,
  google::protobuf::Map<std::string, inference::InferParameter>*
    grpc_parameters) {
  for (const auto& [key, value] : parameters) {
    inference::InferParameter param;
    if (std::holds_alternative<bool>(value)) {
      param.set_bool_param(std::get<bool>(value));
    } else if (std::holds_alternative<double>(value)) {
      param.set_double_param(std::get<double>(value));
    } else if (std::holds_alternative<int32_t>(value)) {
      param.set_int64_param(std::get<int32_t>(value));
    } else {
      param.set_string_param(std::get<std::string>(value));
    }
    grpc_parameters->insert({key, param});
  }
}

InferenceResponse GrpcClient::modelInfer(const InferenceRequest& request) {
  inference::ModelInferRequest grpc_request;
  inference::ModelInferResponse reply;

  ClientContext context;

  grpc_request.set_id(request.getID());

  auto parameters = request.getParameters()->data();
  auto* grpc_parameters = grpc_request.mutable_parameters();
  setParameters(parameters, grpc_parameters);

  auto inputs = request.getInputs();
  for (const auto& input : inputs) {
    inference::ModelInferRequest_InferInputTensor tensor;

    tensor.set_name(input.getName());
    auto shape = input.getShape();
    auto size = 1U;
    for (const auto& index : shape) {
      tensor.add_shape(index);
      size *= index;
    }
    auto datatype = input.getDatatype();
    tensor.set_datatype(types::mapTypeToStr(datatype));
    setParameters(input.getParameters()->data(), tensor.mutable_parameters());

    auto* contents = tensor.mutable_contents();
    // TODO(varunsh): for now, just store as bool
    contents->mutable_bool_contents()->Reserve(size);
    std::memcpy(contents->mutable_bool_contents()->mutable_data(),
                input.getData(), size);
    // switch (datatype){
    //   case types::DataType::BOOL:
    //     contents->mutable_bool_contents()->Reserve(size);
    //     std::memcpy(contents->mutable_bool_contents()->mutable_data(),
    //     input.getData(), size); break;
    //   default:
    //     break;
    // }
  }

  // TODO(varunsh): skipping outputs for now

  auto* stub = this->impl_->getStub();
  Status status = stub->ModelInfer(&context, grpc_request, &reply);

  InferenceResponse response;

  response.setModel(reply.model_name());
  response.setID(reply.id());

  for (const auto& tensor : reply.outputs()) {
    InferenceResponseOutput output;
    output.setName(tensor.name());
    output.setDatatype(types::mapStrToType(tensor.datatype()));
    std::vector<uint64_t> shape;
    shape.reserve(tensor.shape_size());
    auto size = 1U;
    for (const auto& index : tensor.shape()) {
      shape.push_back(static_cast<size_t>(index));
      size *= index;
    }
    output.setShape(shape);
    // TODO(varunsh): skipping parameters for now

    // auto data = std::make_shared<std::byte[]>(size);
    auto data = std::shared_ptr<std::byte>(new std::byte[size],
                                           std::default_delete<std::byte[]>());
    std::memcpy(data.get(), tensor.contents().bool_contents().data(), size);
    output.setData(data);
    response.addOutput(output);
  }

  if (status.ok()) {
    return response;
  } else {
    throw std::runtime_error(status.error_message());
  }
}

void startGrpcServer(int port) {
#ifdef PROTEUS_ENABLE_GRPC
  std::string address = "localhost:" + std::to_string(port);
  grpc::start(address);
#else
  (void)port;  // suppress unused variable warning
#endif
}

void stopGrpcServer() {
#ifdef PROTEUS_ENABLE_GRPC
  grpc::stop();
#endif
}

}  // namespace proteus
