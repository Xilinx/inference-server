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
 * @brief Implements the gRPC server in Proteus
 */

#include "proteus/servers/grpc_server.hpp"

#include <google/protobuf/repeated_ptr_field.h>  // for RepeatedPtrField
#include <grpc/support/log.h>                    // for GPR_ASSERT, GPR_UNL...
#include <grpcpp/grpcpp.h>                       // for Status, InsecureSer...

#include <algorithm>   // for fill, max, transform
#include <cstddef>     // for size_t, byte
#include <cstdint>     // for uint64_t, int16_t
#include <cstring>     // for memcpy
#include <exception>   // for exception
#include <functional>  // for _Bind_helper<>::type
#include <iostream>    // for operator<<, cout
#include <memory>      // for unique_ptr, shared_ptr
#include <stdexcept>   // for invalid_argument
#include <thread>      // for thread, yield
#include <utility>     // for move
#include <vector>      // for vector

#include "predict_api.grpc.pb.h"                  // for GRPCInferenceServic...
#include "predict_api.pb.h"                       // for InferTensorContents
#include "proteus/batching/batcher.hpp"           // for Batcher
#include "proteus/buffers/buffer.hpp"             // for Buffer
#include "proteus/build_options.hpp"              // for PROTEUS_ENABLE_TRACING
#include "proteus/clients/grpc_internal.hpp"      // for mapProtoToParameters
#include "proteus/clients/native.hpp"             // for NativeClient
#include "proteus/core/api.hpp"                   // for modelLoad
#include "proteus/core/data_types.hpp"            // for DataType, mapStrToType
#include "proteus/core/interface.hpp"             // for Interface, Interfac...
#include "proteus/core/manager.hpp"               // for Manager
#include "proteus/core/model_repository.hpp"      // for ModelRepository
#include "proteus/core/predict_api_internal.hpp"  // for InferenceRequestInput
#include "proteus/core/worker_info.hpp"           // for WorkerInfo
#include "proteus/helpers/declarations.hpp"       // for BufferRawPtrs, Infe...
#include "proteus/helpers/string.hpp"             // for toLower
#include "proteus/observation/logging.hpp"        // for Logger
#include "proteus/observation/tracing.hpp"        // for Trace, startTrace

namespace proteus {
class CallDataModelInfer;
class CallDataModelLoad;
class CallDataWorkerLoad;
class CallDataModelReady;
class CallDataModelUnload;
class CallDataWorkerUnload;
class CallDataServerLive;
class CallDataServerMetadata;
class CallDataServerReady;
}  // namespace proteus

// use aliases to prevent clashes between grpc:: and proteus::grpc::
using ServerBuilder = grpc::ServerBuilder;
using ServerCompletionQueue = grpc::ServerCompletionQueue;
template <typename T>
using ServerAsyncResponseWriter = grpc::ServerAsyncResponseWriter<T>;
using ServerContext = grpc::ServerContext;
using Server = grpc::Server;
using StatusCode = grpc::StatusCode;

// namespace inference {
// using StreamModelInferRequest = ModelInferRequest;
// using StreamModelInferResponse = ModelInferResponse;
// }

namespace proteus {

using AsyncService = inference::GRPCInferenceService::AsyncService;

class CallDataBase {
 public:
  virtual void proceed() = 0;
};

template <typename RequestType, typename ReplyType>
class CallData : public CallDataBase {
 public:
  // Take in the "service" instance (in this case representing an asynchronous
  // server) and the completion queue "cq" used for asynchronous communication
  // with the gRPC runtime.
  CallData(AsyncService* service, ::grpc::ServerCompletionQueue* cq)
    : service_(service), cq_(cq), status_(CREATE) {}

  virtual ~CallData() = default;

  void proceed() override {
    if (status_ == CREATE) {
      // Make this instance progress to the PROCESS state.
      status_ = PROCESS;

      waitForRequest();
    } else if (status_ == PROCESS) {
      addNewCallData();

      // queue_->enqueue(this);
      // status_ = WAIT;
      handleRequest();
      status_ = WAIT;
    } else if (status_ == WAIT) {
      std::this_thread::yield();
    } else {
      GPR_ASSERT(status_ == FINISH);
      // Once in the FINISH state, deallocate ourselves (CallData).
      delete this;
    }
  }

  virtual void finish(const ::grpc::Status& status) = 0;

 protected:
  // When we handle a request of this type, we need to tell
  // the completion queue to wait for new requests of the same type.
  virtual void addNewCallData() = 0;

  virtual void waitForRequest() = 0;
  virtual void handleRequest() = 0;

  // The means of communication with the gRPC runtime for an asynchronous
  // server.
  AsyncService* service_;
  // The producer-consumer queue where for asynchronous server notifications.
  ::grpc::ServerCompletionQueue* cq_;
  // Context for the rpc, allowing to tweak aspects of it such as the use
  // of compression, authentication, as well as to send metadata back to the
  // client.
  ::grpc::ServerContext ctx_;

  // What we get from the client.
  RequestType request_;
  // What we send back to the client.
  ReplyType reply_;

  // Let's implement a tiny state machine with the following states.
  enum CallStatus { CREATE, PROCESS, WAIT, FINISH };
  CallStatus status_;  // The current serving state.
};

template <typename RequestType, typename ReplyType>
class CallDataUnary : public CallData<RequestType, ReplyType> {
 public:
  // Take in the "service" instance (in this case representing an asynchronous
  // server) and the completion queue "cq" used for asynchronous communication
  // with the gRPC runtime.
  CallDataUnary(AsyncService* service, ::grpc::ServerCompletionQueue* cq)
    : CallData<RequestType, ReplyType>(service, cq), responder_(&this->ctx_) {}

  void finish(const ::grpc::Status& status = ::grpc::Status::OK) override {
    // And we are done! Let the gRPC runtime know we've finished, using the
    // memory address of this instance as the uniquely identifying tag for
    // the event.
    this->status_ = this->FINISH;
    responder_.Finish(this->reply_, status, this);
  }

 protected:
  // The means to get back to the client.
  ::grpc::ServerAsyncResponseWriter<ReplyType> responder_;
};

template <typename RequestType, typename ReplyType>
class CallDataServerStream : public CallData<RequestType, ReplyType> {
 public:
  // Take in the "service" instance (in this case representing an asynchronous
  // server) and the completion queue "cq" used for asynchronous communication
  // with the gRPC runtime.
  CallDataServerStream(AsyncService* service, ::grpc::ServerCompletionQueue* cq)
    : CallData<RequestType, ReplyType>(service, cq), responder_(&this->ctx_) {}

  void write(const ReplyType& response) { responder_->Write(response, this); }

  void finish(const ::grpc::Status& status = ::grpc::Status::OK) override {
    // And we are done! Let the gRPC runtime know we've finished, using the
    // memory address of this instance as the uniquely identifying tag for
    // the event.
    this->status_ = this->FINISH;
    responder_.Finish(this->reply_, status, this);
  }

 protected:
  // The means to get back to the client.
  ::grpc::ServerAsyncWriter<ReplyType> responder_;
};

template <>
class InferenceRequestInputBuilder<
  inference::ModelInferRequest_InferInputTensor> {
 public:
  static InferenceRequestInput build(
    const inference::ModelInferRequest_InferInputTensor& req,
    Buffer* input_buffer, size_t offset) {
    InferenceRequestInput input;
    input.shared_data_ = nullptr;
    input.name_ = req.name();
    input.shape_.reserve(req.shape_size());
    for (const auto& index : req.shape()) {
      input.shape_.push_back(static_cast<size_t>(index));
    }
    input.dataType_ = DataType(req.datatype().c_str());

    input.parameters_ = mapProtoToParameters(req.parameters());

    auto size = input.getSize();
    auto* dest = static_cast<std::byte*>(input_buffer->data()) + offset;

    const auto tensor = req.contents();
    switch (input.getDatatype()) {
      case DataType::BOOL: {
        std::memcpy(dest, tensor.bool_contents().data(), size * sizeof(char));
        break;
      }
      case DataType::UINT8: {
        const auto* value = tensor.uint_contents().data();
        for (size_t i = 0; i < size; i++) {
          dest[i] = static_cast<std::byte>(value[i]);
        }
        break;
      }
      case DataType::UINT16: {
        const auto* value = tensor.uint_contents().data();
        for (size_t i = 0; i < size; i++) {
          std::memcpy(dest + i, value + i, sizeof(uint16_t));
        }
        break;
      }
      case DataType::UINT32: {
        const auto value = tensor.uint_contents().data();
        std::memcpy(dest, value, size * sizeof(uint32_t));
        break;
      }
      case DataType::UINT64: {
        std::memcpy(dest, tensor.uint64_contents().data(),
                    size * sizeof(uint64_t));
        break;
      }
      case DataType::INT8: {
        const auto* value = tensor.int_contents().data();
        for (size_t i = 0; i < size; i++) {
          dest[i] = static_cast<std::byte>(value[i]);
        }
        break;
      }
      case DataType::INT16: {
        const auto* value = tensor.int_contents().data();
        for (size_t i = 0; i < size; i++) {
          std::memcpy(dest + i, value + i, sizeof(int16_t));
        }
        break;
      }
      case DataType::INT32: {
        std::memcpy(dest, tensor.int_contents().data(), size * sizeof(int32_t));
        break;
      }
      case DataType::INT64: {
        std::memcpy(dest, tensor.int64_contents().data(),
                    size * sizeof(int64_t));
        break;
      }
      case DataType::FP16: {
        // FIXME(varunsh): this is not handled
        std::cout << "Writing FP16 not supported\n";
        break;
      }
      case DataType::FP32: {
        std::memcpy(dest, tensor.fp32_contents().data(), size * sizeof(float));
        break;
      }
      case DataType::FP64: {
        std::memcpy(dest, tensor.fp64_contents().data(), size * sizeof(double));
        break;
      }
      case DataType::STRING: {
        std::memcpy(dest, tensor.bytes_contents().data(),
                    size * sizeof(std::byte));
        break;
      }
      default:
        // TODO(varunsh): what should we do here?
        std::cout << "Unknown datatype\n";
        break;
    }

    input.data_ = dest;
    return input;
  }
};

using InputBuilder =
  InferenceRequestInputBuilder<inference::ModelInferRequest_InferInputTensor>;

#ifdef PROTEUS_ENABLE_LOGGING
#define CALLDATA_IMPL(endpoint, type)                                         \
  class CallData##endpoint                                                    \
    : public CallData##type<inference::endpoint##Request,                     \
                            inference::endpoint##Response> {                  \
   public:                                                                    \
    CallData##endpoint(AsyncService* service, ServerCompletionQueue* cq)      \
      : CallData##type(service, cq) {                                         \
      proceed();                                                              \
    }                                                                         \
                                                                              \
   private:                                                                   \
    Logger logger_{Loggers::kServer};                                         \
                                                                              \
   protected:                                                                 \
    void addNewCallData() override { new CallData##endpoint(service_, cq_); } \
    void waitForRequest() override {                                          \
      service_->Request##endpoint(&ctx_, &request_, &responder_, cq_, cq_,    \
                                  this);                                      \
    }                                                                         \
    void handleRequest() override
#else
#define CALLDATA_IMPL(endpoint, type)                                         \
  class CallData##endpoint                                                    \
    : public CallData##type<inference::endpoint##Request,                     \
                            inference::endpoint##Response> {                  \
   public:                                                                    \
    CallData##endpoint(AsyncService* service, ServerCompletionQueue* cq)      \
      : CallData##type(service, cq) {                                         \
      proceed();                                                              \
    }                                                                         \
                                                                              \
   protected:                                                                 \
    void addNewCallData() override { new CallData##endpoint(service_, cq_); } \
    void waitForRequest() override {                                          \
      service_->Request##endpoint(&ctx_, &request_, &responder_, cq_, cq_,    \
                                  this);                                      \
    }                                                                         \
    void handleRequest() override
#endif

#define CALLDATA_IMPL_END \
  }                       \
  ;

CALLDATA_IMPL(ModelInfer, Unary);

public:
const inference::ModelInferRequest& getRequest() const {
  return this->request_;
}

inference::ModelInferResponse& getReply() { return this->reply_; }
CALLDATA_IMPL_END

template <>
class InferenceRequestBuilder<CallDataModelInfer*> {
 public:
  static InferenceRequestPtr build(
    const CallDataModelInfer* req, size_t& buffer_index,
    const std::vector<BufferRawPtrs>& input_buffers,
    std::vector<size_t>& input_offsets,
    const std::vector<BufferRawPtrs>& output_buffers,
    std::vector<size_t>& output_offsets, const size_t& batch_size,
    size_t& batch_offset) {
    auto request = std::make_shared<InferenceRequest>();
    const auto& grpc_request = req->getRequest();

    request->id_ = grpc_request.id();

    request->parameters_ = mapProtoToParameters(grpc_request.parameters());

    request->callback_ = nullptr;

    auto buffer_index_backup = buffer_index;
    auto batch_offset_backup = batch_offset;

    for (const auto& input : grpc_request.inputs()) {
      auto buffers = input_buffers[buffer_index];
      for (auto& buffer : buffers) {
        auto& offset = input_offsets[buffer_index];

        request->inputs_.push_back(InputBuilder::build(input, buffer, offset));
        const auto& last_input = request->inputs_.back();
        offset += (last_input.getSize() * last_input.getDatatype().size());
      }
      batch_offset++;
      if (batch_offset == batch_size) {
        batch_offset = 0;
        buffer_index++;
        // std::fill(input_offsets.begin(), input_offsets.end(), 0);
      }
    }

    // TODO(varunsh): output_offset is currently ignored! The size of the
    // output needs to come from the worker but we have no such information.
    buffer_index = buffer_index_backup;
    batch_offset = batch_offset_backup;

    if (grpc_request.outputs_size() != 0) {
      for (const auto& output : grpc_request.outputs()) {
        // TODO(varunsh): we're ignoring incoming output data
        (void)output;
        auto buffers = output_buffers[buffer_index];
        for (auto& buffer : buffers) {
          auto& offset = output_offsets[buffer_index];

          request->outputs_.emplace_back();
          request->outputs_.back().setData(
            static_cast<std::byte*>(buffer->data()) + offset);
        }
        batch_offset++;
        if (batch_offset == batch_size) {
          batch_offset = 0;
          buffer_index++;
          std::fill(output_offsets.begin(), output_offsets.end(), 0);
        }
      }
    } else {
      for (const auto& input : grpc_request.inputs()) {
        (void)input;  // suppress unused variable warning
        auto buffers = output_buffers[buffer_index];
        for (size_t j = 0; j < buffers.size(); j++) {
          auto& buffer = buffers[j];
          const auto& offset = output_offsets[buffer_index];

          request->outputs_.emplace_back();
          request->outputs_.back().setData(
            static_cast<std::byte*>(buffer->data()) + offset);
        }
        batch_offset++;
        if (batch_offset == batch_size) {
          batch_offset = 0;
          buffer_index++;
          std::fill(output_offsets.begin(), output_offsets.end(), 0);
        }
      }
    }

    return request;
  }
};

using RequestBuilder = InferenceRequestBuilder<CallDataModelInfer*>;

// CALLDATA_IMPL(StreamModelInfer, ServerStream);

//  public:
//   const inference::ModelInferRequest& getRequest() const {
//     return this->request_;
//   }
// CALLDATA_IMPL_END

void grpcUnaryCallback(CallDataModelInfer* calldata,
                       const InferenceResponse& response) {
  if (response.isError()) {
    calldata->finish(::grpc::Status(StatusCode::UNKNOWN, response.getError()));
    return;
  }
  try {
    mapResponsetoProto(response, calldata->getReply());
  } catch (const invalid_argument& e) {
    calldata->finish(::grpc::Status(StatusCode::UNKNOWN, e.what()));
    return;
  }

  // #ifdef PROTEUS_ENABLE_TRACING
  //   const auto &context = response.getContext();
  //   propagate(resp.get(), context);
  // #endif
  calldata->finish();
}

class GrpcApiUnary : public Interface {
 public:
  /**
   * @brief Construct a new DrogonHttp object
   *
   * @param req
   * @param callback
   */
  explicit GrpcApiUnary(CallDataModelInfer* calldata) : calldata_(calldata) {
    this->type_ = InterfaceType::kGrpc;
  }

  std::shared_ptr<InferenceRequest> getRequest(
    size_t& buffer_index, const std::vector<BufferRawPtrs>& input_buffers,
    std::vector<size_t>& input_offsets,
    const std::vector<BufferRawPtrs>& output_buffers,
    std::vector<size_t>& output_offsets, const size_t& batch_size,
    size_t& batch_offset) override {
#ifdef PROTEUS_ENABLE_LOGGING
    const auto& logger = this->getLogger();
#endif
    try {
      auto request = RequestBuilder::build(
        this->calldata_, buffer_index, input_buffers, input_offsets,
        output_buffers, output_offsets, batch_size, batch_offset);
      Callback callback =
        std::bind(grpcUnaryCallback, this->calldata_, std::placeholders::_1);
      request->setCallback(std::move(callback));
      return request;
    } catch (const invalid_argument& e) {
      PROTEUS_LOG_INFO(logger, e.what());
      errorHandler(e);
      return nullptr;
    }
  }

  size_t getInputSize() override {
    return calldata_->getRequest().inputs_size();
  }

  void errorHandler(const std::exception& e) override {
    PROTEUS_LOG_INFO(this->getLogger(), e.what());
    calldata_->finish(::grpc::Status(StatusCode::NOT_FOUND, e.what()));
  }

 private:
  CallDataModelInfer* calldata_;
};

CALLDATA_IMPL(ServerLive, Unary) {
  reply_.set_live(true);
  finish();
}
CALLDATA_IMPL_END

CALLDATA_IMPL(ServerReady, Unary) {
  reply_.set_ready(true);
  finish();
}
CALLDATA_IMPL_END

CALLDATA_IMPL(ModelReady, Unary) {
  const auto& model = request_.name();
  try {
    reply_.set_ready(Manager::getInstance().workerReady(model));
    finish();
  } catch (const invalid_argument& e) {
    reply_.set_ready(false);
    finish(::grpc::Status(StatusCode::NOT_FOUND, e.what()));
  }
}
CALLDATA_IMPL_END

CALLDATA_IMPL(ServerMetadata, Unary) {
  auto metadata = serverMetadata();
  reply_.set_name(metadata.name);
  reply_.set_version(metadata.version);
  for (const auto& extension : metadata.extensions) {
    reply_.add_extensions(extension);
  }
  finish();
}
CALLDATA_IMPL_END

CALLDATA_IMPL(ModelList, Unary) {
  NativeClient client;
  auto models = client.modelList();
  for (const auto& model : models) {
    reply_.add_models(model);
  }
  finish();
}
CALLDATA_IMPL_END

CALLDATA_IMPL(ModelLoad, Unary) {
  auto parameters = mapProtoToParameters(request_.parameters());

  auto* model = request_.mutable_name();
  toLower(model);
  try {
    ::proteus::modelLoad(*model, parameters.get());
  } catch (const runtime_error& e) {
    PROTEUS_LOG_ERROR(logger_, e.what());
    finish(::grpc::Status(StatusCode::NOT_FOUND, e.what()));
    return;
  }

  finish();
}
CALLDATA_IMPL_END

CALLDATA_IMPL(ModelUnload, Unary) {
  auto* model = request_.mutable_name();
  toLower(model);
  ::proteus::modelUnload(*model);
  finish();
}
CALLDATA_IMPL_END

CALLDATA_IMPL(WorkerLoad, Unary) {
  auto parameters = mapProtoToParameters(request_.parameters());

  auto* model = request_.mutable_name();
  toLower(model);

  try {
    auto endpoint = ::proteus::workerLoad(*model, parameters.get());
    reply_.set_endpoint(endpoint);
    finish();
  } catch (const runtime_error& e) {
    PROTEUS_LOG_ERROR(logger_, e.what());
    finish(::grpc::Status(StatusCode::NOT_FOUND, e.what()));
    return;
  }
}
CALLDATA_IMPL_END

CALLDATA_IMPL(WorkerUnload, Unary) {
  auto* worker = request_.mutable_name();
  toLower(worker);
  ::proteus::workerUnload(*worker);
  finish();
}
CALLDATA_IMPL_END

void CallDataModelInfer::handleRequest() {
  const auto& model = request_.model_name();
#ifdef PROTEUS_ENABLE_TRACING
  auto trace = startTrace(&(__func__[0]));
  trace->setAttribute("model", model);
  trace->startSpan("request_handler");
#endif

  try {
    auto request = std::make_unique<GrpcApiUnary>(this);
#ifdef PROTEUS_ENABLE_TRACING
    trace->endSpan();
    request->setTrace(std::move(trace));
#endif
    ::proteus::modelInfer(model, std::move(request));
  } catch (const invalid_argument& e) {
    PROTEUS_LOG_INFO(logger_, e.what());
    finish(::grpc::Status(StatusCode::NOT_FOUND, e.what()));
    return;
  }
}

class GrpcServer final {
 public:
  /// Get the singleton GrpcServer instance
  static GrpcServer& getInstance() { return create("", -1); }

  static GrpcServer& create(const std::string& address, const int cq_count) {
    static GrpcServer server(address, cq_count);
    return server;
  }

  GrpcServer(GrpcServer const&) = delete;  ///< Copy constructor
  GrpcServer& operator=(const GrpcServer&) =
    delete;                                 ///< Copy assignment constructor
  GrpcServer(GrpcServer&& other) = delete;  ///< Move constructor
  GrpcServer& operator=(GrpcServer&& other) =
    delete;  ///< Move assignment constructor

  ~GrpcServer() {
    server_->Shutdown();
    // Always shutdown the completion queues after the server.
    for (auto& cq : cq_) {
      cq->Shutdown();
      // drain the completion queue to prevent assertion errors in grpc
      void* tag = nullptr;
      bool ok = false;
      while (cq->Next(&tag, &ok)) {
      }
    }
  }

 private:
  GrpcServer(const std::string& address, const int cq_count) {
    ServerBuilder builder;
    builder.SetMaxReceiveMessageSize(kMaxGrpcMessageSize);
    builder.SetMaxSendMessageSize(kMaxGrpcMessageSize);
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(address, ::grpc::InsecureServerCredentials());
    // Register "service_" as the instance through which we'll communicate
    // with clients. In this case it corresponds to an *asynchronous* service.
    builder.RegisterService(&service_);
    // Get hold of the completion queue used for the asynchronous
    // communication with the gRPC runtime.
    for (auto i = 0; i < cq_count; i++) {
      cq_.push_back(builder.AddCompletionQueue());
    }
    // Finally assemble the server.
    server_ = builder.BuildAndStart();

    // Start threads to handle incoming RPCs
    for (auto i = 0; i < cq_count; i++) {
      threads_.emplace_back(&GrpcServer::handleRpcs, this, i);
      // just detach threads for now to simplify shutdown
      threads_.back().detach();
    }
  }

  // This can be run in multiple threads if needed.
  void handleRpcs(int index) {
    auto& my_cq = cq_.at(index);

    // Spawn a new CallData instance to serve new clients.
    new CallDataServerLive(&service_, my_cq.get());
    new CallDataServerMetadata(&service_, my_cq.get());
    new CallDataServerReady(&service_, my_cq.get());
    new CallDataModelList(&service_, my_cq.get());
    new CallDataModelReady(&service_, my_cq.get());
    new CallDataModelLoad(&service_, my_cq.get());
    new CallDataModelUnload(&service_, my_cq.get());
    new CallDataWorkerLoad(&service_, my_cq.get());
    new CallDataWorkerUnload(&service_, my_cq.get());
    new CallDataModelInfer(&service_, my_cq.get());
    // new CallDataStreamModelInfer(&service_, my_cq.get());
    void* tag;  // uniquely identifies a request.
    bool ok;
    while (true) {
      // Block waiting to read the next event from the completion queue. The
      // event is uniquely identified by its tag, which in this case is the
      // memory address of a CallDataBase instance.
      // The return value of Next should always be checked. This return value
      // tells us whether there is any kind of event or cq_ is shutting down.
      auto event_received = my_cq->Next(&tag, &ok);
      if (GPR_UNLIKELY(!(ok && event_received))) {
        break;
      }
      static_cast<CallDataBase*>(tag)->proceed();
    }
  }

  std::vector<std::unique_ptr<::grpc::ServerCompletionQueue>> cq_;
  inference::GRPCInferenceService::AsyncService service_;
  std::unique_ptr<::grpc::Server> server_;
  std::vector<std::thread> threads_;
  ModelRepository repository_;
};

namespace grpc {

void start(int port) {
  const std::string address = "0.0.0.0:" + std::to_string(port);
  GrpcServer::create(address, 1);
}

void stop() {
  // the GrpcServer's destructor is called automatically
  // auto& foo = GrpcServer::getInstance();
  // foo.~GrpcServer();
}

}  // namespace grpc

}  // namespace proteus
