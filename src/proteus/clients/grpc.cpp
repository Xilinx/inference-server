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

#include <google/protobuf/repeated_ptr_field.h>  // for RepeatedPtrField
#include <google/protobuf/stubs/common.h>        // for string
#include <grpcpp/grpcpp.h>                       // for ClientContext, Creat...

#include <cstddef>    // for byte, size_t
#include <cstdint>    // for uint64_t, uint32_t
#include <cstring>    // for memcpy
#include <iostream>   // for operator<<, cout
#include <map>        // for map
#include <memory>     // for make_shared, reinter...
#include <set>        // for set
#include <stdexcept>  // for runtime_error
#include <string>     // for string, operator+
#include <utility>    // for move
#include <variant>    // for visit
#include <vector>     // for vector

#include "predict_api.grpc.pb.h"             // for GRPCInferenceService...
#include "predict_api.pb.h"                  // for InferTensorContents
#include "proteus/build_options.hpp"         // for PROTEUS_ENABLE_GRPC
#include "proteus/core/data_types.hpp"       // for DataType, DataType::...
#include "proteus/helpers/declarations.hpp"  // for InferenceResponseOutput
#include "proteus/servers/grpc_server.hpp"   // for start, stop

using grpc::ClientContext;
using grpc::Status;

namespace proteus {

using types::DataType;

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

ServerMetadata GrpcClient::serverMetadata() {
  inference::ServerMetadataRequest request;
  inference::ServerMetadataResponse reply;

  ClientContext context;

  auto* stub = this->impl_->getStub();
  Status status = stub->ServerMetadata(&context, request, &reply);

  if (status.ok()) {
    auto ext = reply.extensions();
    std::set<std::string> extensions(ext.begin(), ext.end());
    ServerMetadata metadata{reply.name(), reply.version(), extensions};
    return metadata;
  }
  throw std::runtime_error(status.error_message());
}

bool GrpcClient::serverLive() {
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
  throw std::runtime_error(status.error_message());
}

bool GrpcClient::serverReady() {
  inference::ServerReadyRequest request;
  inference::ServerReadyResponse reply;

  ClientContext context;

  auto* stub = this->impl_->getStub();
  Status status = stub->ServerReady(&context, request, &reply);

  if (status.ok()) {
    return reply.ready();
  }
  throw std::runtime_error(status.error_message());
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
  }
  throw std::invalid_argument(status.error_message());
}

std::vector<std::string> GrpcClient::modelList() {
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
  throw std::invalid_argument(status.error_message());
}

// refer to cppreference for std::visit
// helper type for the visitor #4
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void mapParametersToProto(
  const std::map<std::string, proteus::Parameter>& parameters,
  google::protobuf::Map<std::string, inference::InferParameter>*
    grpc_parameters) {
  for (const auto& [key, value] : parameters) {
    inference::InferParameter param;
    std::visit(
      overloaded{[&](bool arg) { param.set_bool_param(arg); },
                 [&](double arg) { param.set_double_param(arg); },
                 [&](int32_t arg) { param.set_int64_param(arg); },
                 [&](const std::string& arg) { param.set_string_param(arg); }},
      value);
    grpc_parameters->insert({key, param});
  }
}

std::string GrpcClient::modelLoad(const std::string& model,
                                  RequestParameters* parameters) {
  inference::ModelLoadRequest request;
  inference::ModelLoadResponse reply;

  ClientContext context;

  request.set_name(model);
  auto* params = request.mutable_parameters();
  if (parameters != nullptr) {
    mapParametersToProto(parameters->data(), params);
  }

  auto* stub = this->impl_->getStub();
  Status status = stub->ModelLoad(&context, request, &reply);

  if (status.ok()) {
    return reply.endpoint();
  }
  throw std::runtime_error(status.error_message());
}

void GrpcClient::modelUnload(const std::string& model) {
  inference::ModelUnloadRequest request;
  inference::ModelUnloadResponse reply;

  ClientContext context;

  request.set_name(model);

  auto* stub = this->impl_->getStub();
  Status status = stub->ModelUnload(&context, request, &reply);

  if (!status.ok()) {
    throw std::runtime_error(status.error_message());
  }
}

void mapRequestToProto(const InferenceRequest& request,
                       inference::ModelInferRequest& grpc_request) {
  grpc_request.set_id(request.getID());

  auto* parameters = request.getParameters();
  if (parameters != nullptr) {
    auto params = parameters->data();
    auto* grpc_parameters = grpc_request.mutable_parameters();
    mapParametersToProto(params, grpc_parameters);
  }

  auto inputs = request.getInputs();
  for (const auto& input : inputs) {
    auto* tensor = grpc_request.add_inputs();

    tensor->set_name(input.getName());
    auto shape = input.getShape();
    auto size = 1U;
    for (const auto& index : shape) {
      tensor->add_shape(index);
      size *= index;
    }
    auto datatype = input.getDatatype();
    tensor->set_datatype(types::mapTypeToStr(datatype));
    mapParametersToProto(input.getParameters()->data(),
                         tensor->mutable_parameters());

    switch (input.getDatatype()) {
      case DataType::BOOL: {
        auto* data = static_cast<bool*>(input.getData());
        auto* contents = tensor->mutable_contents()->mutable_bool_contents();
        for (size_t i = 0; i < input.getSize(); i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::UINT8: {
        auto* data = static_cast<uint8_t*>(input.getData());
        auto* contents = tensor->mutable_contents()->mutable_uint_contents();
        for (size_t i = 0; i < input.getSize(); i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::UINT16: {
        auto* data = static_cast<uint16_t*>(input.getData());
        auto* contents = tensor->mutable_contents()->mutable_uint_contents();
        for (size_t i = 0; i < input.getSize(); i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::UINT32: {
        auto* data = static_cast<uint32_t*>(input.getData());
        auto* contents = tensor->mutable_contents()->mutable_uint_contents();
        for (size_t i = 0; i < input.getSize(); i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::UINT64: {
        auto* data = static_cast<uint64_t*>(input.getData());
        auto* contents = tensor->mutable_contents()->mutable_uint64_contents();
        for (size_t i = 0; i < input.getSize(); i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::INT8: {
        auto* data = static_cast<int8_t*>(input.getData());
        auto* contents = tensor->mutable_contents()->mutable_int_contents();
        for (size_t i = 0; i < input.getSize(); i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::INT16: {
        auto* data = static_cast<int16_t*>(input.getData());
        auto* contents = tensor->mutable_contents()->mutable_int_contents();
        for (size_t i = 0; i < input.getSize(); i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::INT32: {
        auto* data = static_cast<int32_t*>(input.getData());
        auto* contents = tensor->mutable_contents()->mutable_int_contents();
        for (size_t i = 0; i < input.getSize(); i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::INT64: {
        auto* data = static_cast<int64_t*>(input.getData());
        auto* contents = tensor->mutable_contents()->mutable_int64_contents();
        for (size_t i = 0; i < input.getSize(); i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::FP16: {
        // FIXME(varunsh): this is not handled
        std::cout << "Writing FP16 not supported\n";
        break;
      }
      case DataType::FP32: {
        auto* data = static_cast<float*>(input.getData());
        auto* contents = tensor->mutable_contents()->mutable_fp32_contents();
        for (size_t i = 0; i < input.getSize(); i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::FP64: {
        auto* data = static_cast<double*>(input.getData());
        auto* contents = tensor->mutable_contents()->mutable_fp64_contents();
        for (size_t i = 0; i < input.getSize(); i++) {
          contents->Add(data[i]);
        }
        break;
      }
      case DataType::STRING: {
        auto* data = static_cast<std::string*>(input.getData());
        auto* contents = tensor->mutable_contents()->mutable_bytes_contents();
        contents->Add(std::move(*data));
        // for(size_t i = 0; i < output.getSize(); i++){
        //   contents->Add(data->data()[i]);
        // }
        break;
      }
      default:
        // TODO(varunsh): what should we do here?
        std::cout << "Unknown datatype\n";
        break;
    }
  }

  // TODO(varunsh): skipping outputs for now
}

void mapPrototoResponse(const inference::ModelInferResponse& reply,
                        InferenceResponse& response) {
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

    switch (output.getDatatype()) {
      case DataType::BOOL: {
        auto data = std::make_shared<std::vector<char>>();
        data->resize(size);
        std::memcpy(data->data(), tensor.contents().bool_contents().data(),
                    size * sizeof(char));
        output.setData(std::reinterpret_pointer_cast<std::byte>(data));
        break;
      }
      case DataType::UINT8:
      case DataType::UINT16:
      case DataType::UINT32: {
        auto data = std::make_shared<std::vector<uint32_t>>();
        data->resize(size);
        std::memcpy(data->data(), tensor.contents().uint_contents().data(),
                    size * sizeof(uint32_t));
        output.setData(std::reinterpret_pointer_cast<std::byte>(data));
        output.setDatatype(DataType::UINT32);
        break;
      }
      case DataType::UINT64: {
        auto data = std::make_shared<std::vector<uint64_t>>();
        data->resize(size);
        std::memcpy(data->data(), tensor.contents().uint64_contents().data(),
                    size * sizeof(uint64_t));
        output.setData(std::reinterpret_pointer_cast<std::byte>(data));
        break;
      }
      case DataType::INT8:
      case DataType::INT16:
      case DataType::INT32: {
        auto data = std::make_shared<std::vector<int32_t>>();
        data->resize(size);
        std::memcpy(data->data(), tensor.contents().int_contents().data(),
                    size * sizeof(int32_t));
        output.setData(std::reinterpret_pointer_cast<std::byte>(data));
        output.setDatatype(DataType::INT32);
        break;
      }
      case DataType::INT64: {
        auto data = std::make_shared<std::vector<int64_t>>();
        data->resize(size);
        std::memcpy(data->data(), tensor.contents().int64_contents().data(),
                    size * sizeof(int64_t));
        output.setData(std::reinterpret_pointer_cast<std::byte>(data));
        break;
      }
      case DataType::FP16: {
        // FIXME(varunsh): this is not handled
        std::cout << "Writing FP16 not supported\n";
        break;
      }
      case DataType::FP32: {
        auto data = std::make_shared<std::vector<float>>();
        data->resize(size);
        std::memcpy(data->data(), tensor.contents().fp32_contents().data(),
                    size * sizeof(float));
        output.setData(std::reinterpret_pointer_cast<std::byte>(data));
        break;
      }
      case DataType::FP64: {
        auto data = std::make_shared<std::vector<double>>();
        data->resize(size);
        std::memcpy(data->data(), tensor.contents().fp64_contents().data(),
                    size * sizeof(double));
        output.setData(std::reinterpret_pointer_cast<std::byte>(data));
        break;
      }
      case DataType::STRING: {
        auto data = std::make_shared<std::vector<std::byte>>();
        data->resize(size);
        std::memcpy(data->data(), tensor.contents().bytes_contents().data(),
                    size * sizeof(std::byte));
        output.setData(std::reinterpret_pointer_cast<std::byte>(data));
        break;
      }
      default:
        // TODO(varunsh): what should we do here?
        std::cout << "Unknown datatype\n";
        break;
    }

    // auto data = std::make_shared<std::byte[]>(size);
    // auto data = std::shared_ptr<std::byte>(new std::byte[size],
    //                                        std::default_delete<std::byte[]>());
    // std::memcpy(data.get(), tensor.contents().bool_contents().data(), size);
    // auto data = std::make_shared<std::vector<uint64_t>>();
    // data->push_back(tensor.contents().int64_contents(0));
    // output.setData(std::reinterpret_pointer_cast<std::byte>(data));
    response.addOutput(output);
  }
}

InferenceResponse GrpcClient::modelInfer(const std::string& model,
                                         const InferenceRequest& request) {
  inference::ModelInferRequest grpc_request;
  inference::ModelInferResponse reply;

  ClientContext context;

  grpc_request.set_model_name(model);
  mapRequestToProto(request, grpc_request);

  auto* stub = this->impl_->getStub();
  Status status = stub->ModelInfer(&context, grpc_request, &reply);

  if (!status.ok()) {
    throw std::runtime_error(status.error_message());
  }

  InferenceResponse response;
  mapPrototoResponse(reply, response);
  return response;
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
