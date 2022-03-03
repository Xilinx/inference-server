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

#include "predict_api.grpc.pb.h"
#include "proteus/batching/batcher.hpp"
#include "proteus/clients/grpc_internal.hpp"
#include "proteus/core/interface.hpp"
#include "proteus/core/manager.hpp"
#include "proteus/core/predict_api_internal.hpp"
#include "proteus/core/worker_info.hpp"
#include "proteus/helpers/queue.hpp"
#include "proteus/observation/logging.hpp"
#include "proteus/version.hpp"

// use aliases to prevent clashes between grpc:: and proteus::grpc::
using ServerBuilder = grpc::ServerBuilder;
using ServerCompletionQueue = grpc::ServerCompletionQueue;
template <typename T>
using ServerAsyncResponseWriter = grpc::ServerAsyncResponseWriter<T>;
using ServerContext = grpc::ServerContext;
using Server = grpc::Server;
using Status = grpc::Status;
using StatusCode = grpc::StatusCode;

namespace proteus {

#define CALLDATA_IMPL(endpoint)                                               \
  class CallData##endpoint : public CallData<inference::endpoint##Request,    \
                                             inference::endpoint##Response> { \
   public:                                                                    \
    CallData##endpoint(AsyncService* service, ServerCompletionQueue* cq)      \
      : CallData(service, cq) {                                               \
      proceed();                                                              \
    };                                                                        \
                                                                              \
   protected:                                                                 \
    void addNewCallData() override { new CallData##endpoint(service_, cq_); } \
    void waitForRequest() override {                                          \
      service_->Request##endpoint(&ctx_, &request_, &responder_, cq_, cq_,    \
                                  this);                                      \
    }                                                                         \
    void handleRequest() override

#define CALLDATA_IMPL_END \
  }                       \
  ;

CALLDATA_IMPL(ServerLive) {
  reply_.set_live(true);
  finish();
}
CALLDATA_IMPL_END

CALLDATA_IMPL(ServerReady) {
  reply_.set_ready(true);
  finish();
}
CALLDATA_IMPL_END

CALLDATA_IMPL(ModelReady) {
  auto& model = request_.name();
  try {
    if (!Manager::getInstance().workerReady(model)) {
      reply_.set_ready(false);
    }
    finish();
  } catch (const std::invalid_argument& e) {
    reply_.set_ready(false);
    finish(Status(StatusCode::NOT_FOUND, e.what()));
  }
}
CALLDATA_IMPL_END

void parseResponse(CallDataModelInfer* calldata, InferenceResponse response) {
  auto& reply = calldata->getReply();
  reply.set_model_name(response.getModel());
  reply.set_id(response.getID());
  auto outputs = response.getOutputs();
  for (InferenceResponseOutput& output : outputs) {
    auto* tensor = reply.add_outputs();
    tensor->set_name(output.getName());
    // auto* parameters = tensor->mutable_parameters();
    tensor->set_datatype(types::mapTypeToStr(output.getDatatype()));
    auto shape = output.getShape();
    auto size = 1U;
    for (const size_t& index : shape) {
      tensor->add_shape(index);
      size *= index;
    }
    auto* contents = tensor->mutable_contents()->mutable_bool_contents();
    contents->Reserve(size);
    std::memcpy(contents->mutable_data(), output.getData(), size);
  }
}

void grpcCallback(CallDataModelInfer* calldata,
                  const InferenceResponse& response) {
  if (response.isError()) {
    calldata->finish(Status(StatusCode::UNKNOWN, response.getError()));
    return;
  } else {
    try {
      parseResponse(calldata, response);
    } catch (const std::invalid_argument& e) {
      calldata->finish(Status(StatusCode::UNKNOWN, e.what()));
      return;
    }
  }
  // #ifdef PROTEUS_ENABLE_TRACING
  //   const auto &context = response.getContext();
  //   propagate(resp.get(), context);
  // #endif
  calldata->finish();
}

class GrpcApi : public Interface {
 public:
  /**
   * @brief Construct a new DrogonHttp object
   *
   * @param req
   * @param callback
   */
  GrpcApi(CallDataModelInfer* calldata) : calldata_(calldata){};

  std::shared_ptr<InferenceRequest> getRequest(
    size_t& buffer_index, const std::vector<BufferRawPtrs>& input_buffers,
    std::vector<size_t>& input_offsets,
    const std::vector<BufferRawPtrs>& output_buffers,
    std::vector<size_t>& output_offsets, const size_t& batch_size,
    size_t& batch_offset) override {
    try {
      auto request = RequestBuilder::build(
        this->calldata_, buffer_index, input_buffers, input_offsets,
        output_buffers, output_offsets, batch_size, batch_offset);
      Callback callback =
        std::bind(grpcCallback, this->calldata_, std::placeholders::_1);
      request->setCallback(std::move(callback));
      return request;
    } catch (const std::invalid_argument& e) {
      SPDLOG_LOGGER_INFO(this->logger_, e.what());
      errorHandler(e);
      return nullptr;
    }
  }

  size_t getInputSize() override {
    return calldata_->getRequest().inputs_size();
  }

  void errorHandler(const std::invalid_argument& e) override {
    SPDLOG_INFO(e.what());
    calldata_->finish(Status(StatusCode::NOT_FOUND, e.what()));
  }

 private:
  CallDataModelInfer* calldata_;
};

CALLDATA_IMPL(ServerMetadata) {
  reply_.set_name("proteus");
  reply_.set_version(kProteusVersion);
#ifdef PROTEUS_ENABLE_AKS
  reply_.add_extensions("aks");
#endif
#ifdef PROTEUS_ENABLE_VITIS
  reply_.add_extensions("vitis");
#endif
  finish();
}
CALLDATA_IMPL_END

CALLDATA_IMPL(ModelLoad) {
  auto parameters = addParameters(request_.parameters());

  const std::string& model = request_.name();

  auto hyphen_pos = model.find('-');
  std::string name;
  // if there's a hyphen in the name, currently assuming it's for xmodel. So,
  // extract the first part as the worker and the second part as the xmodel file
  // name. Put that information into the parameters with the default path for
  // KServe (/mnt/models)
  if (hyphen_pos != std::string::npos) {
    name = model.substr(0, hyphen_pos);
    auto xmodel = model.substr(hyphen_pos + 1, model.length() - hyphen_pos);
    parameters->put("xmodel",
                    "/mnt/models/" + model + "/" + xmodel + ".xmodel");
  } else {
    name = model;
  }

  std::string endpoint;
  try {
    endpoint = Manager::getInstance().loadWorker(name, *parameters);
  } catch (const std::exception& e) {
    SPDLOG_ERROR(e.what());
    finish(Status(StatusCode::NOT_FOUND, e.what()));
    return;
  }

  reply_.set_endpoint(endpoint);
  finish();
}
CALLDATA_IMPL_END

CALLDATA_IMPL(ModelUnload) {
  const auto& name = request_.name();

  Manager::getInstance().unloadWorker(name);
  finish();
}
CALLDATA_IMPL_END

CallDataModelInfer::CallDataModelInfer(AsyncService* service,
                                       ServerCompletionQueue* cq)
  : CallData(service, cq) {
  proceed();
}

void CallDataModelInfer::addNewCallData() {
  new CallDataModelInfer(service_, cq_);
}

void CallDataModelInfer::waitForRequest() {
  service_->RequestModelInfer(&ctx_, &request_, &responder_, cq_, cq_, this);
}

void CallDataModelInfer::handleRequest() {
  auto& model = request_.model_name();

  WorkerInfo* worker = nullptr;
  try {
    worker = Manager::getInstance().getWorker(model);
  } catch (const std::invalid_argument& e) {
    SPDLOG_INFO(e.what());
    finish(Status(StatusCode::NOT_FOUND, "Worker " + model + " not found"));
    return;
  }

  auto request = std::make_unique<GrpcApi>(this);
  // #ifdef PROTEUS_ENABLE_METRICS
  //   request->set_time(now);
  // #endif
  auto* batcher = worker->getBatcher();
  // #ifdef PROTEUS_ENABLE_TRACING
  //   trace->endSpan();
  //   request->setTrace(std::move(trace));
  // #endif
  batcher->enqueue(std::move(request));
}

const inference::ModelInferRequest& CallDataModelInfer::getRequest() const {
  return this->request_;
}

inference::ModelInferResponse& CallDataModelInfer::getReply() {
  return this->reply_;
}

class GrpcServer final {
 public:
  /// Get the singleton GrpcServer instance
  static GrpcServer& getInstance() { return create("", -1); };

  static GrpcServer& create(const std::string& address, const int cq_count) {
    static GrpcServer server(address, cq_count);
    return server;
  };

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
    }
  };

 private:
  GrpcServer(const std::string& address, const int cq_count) {
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(address, ::grpc::InsecureServerCredentials());
    // Register "service_" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *asynchronous* service.
    builder.RegisterService(&service_);
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
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
  };

  // This can be run in multiple threads if needed.
  void handleRpcs(int index) {
    auto& my_cq = cq_.at(index);

    // Spawn a new CallData instance to serve new clients.
    new CallDataServerLive(&service_, my_cq.get());
    new CallDataServerMetadata(&service_, my_cq.get());
    new CallDataServerReady(&service_, my_cq.get());
    new CallDataModelReady(&service_, my_cq.get());
    new CallDataModelLoad(&service_, my_cq.get());
    new CallDataModelUnload(&service_, my_cq.get());
    new CallDataModelInfer(&service_, my_cq.get());
    void* tag;  // uniquely identifies a request.
    bool ok;
    while (true) {
      // Block waiting to read the next event from the completion queue. The
      // event is uniquely identified by its tag, which in this case is the
      // memory address of a CallDataBase instance.
      // The return value of Next should always be checked. This return value
      // tells us whether there is any kind of event or cq_ is shutting down.
      GPR_ASSERT(my_cq->Next(&tag, &ok));
      if (GPR_UNLIKELY(!(ok))) {
        break;
      }
      static_cast<CallDataBase*>(tag)->proceed();
    }
  };

  std::vector<std::unique_ptr<::grpc::ServerCompletionQueue>> cq_;
  inference::GRPCInferenceService::AsyncService service_;
  std::unique_ptr<::grpc::Server> server_;
  std::vector<std::thread> threads_;
};

namespace grpc {

void start(const std::string& address) { GrpcServer::create(address, 1); }

void stop() {
  // the GrpcServer's destructor is called automatically
  // auto& foo = GrpcServer::getInstance();
  // foo.~GrpcServer();
}

}  // namespace grpc

}  // namespace proteus
