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
 * @brief Defines the gRPC server in Proteus
 */

#ifndef GUARD_PROTEUS_SERVERS_GRPC_SERVER
#define GUARD_PROTEUS_SERVERS_GRPC_SERVER

#include "proteus/build_options.hpp"

#ifdef PROTEUS_ENABLE_GRPC

#include <grpcpp/grpcpp.h>

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "predict_api.grpc.pb.h"

namespace proteus {

using AsyncService = inference::GRPCInferenceService::AsyncService;

class CallDataBase {
 public:
  virtual void proceed() = 0;
};

using AsyncService = inference::GRPCInferenceService::AsyncService;

template <typename RequestType, typename ReplyType>
class CallData : public CallDataBase {
 public:
  // Take in the "service" instance (in this case representing an asynchronous
  // server) and the completion queue "cq" used for asynchronous communication
  // with the gRPC runtime.
  CallData(AsyncService* service, ::grpc::ServerCompletionQueue* cq)
    : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {}

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

  void finish(const ::grpc::Status& status = ::grpc::Status::OK) {
    // And we are done! Let the gRPC runtime know we've finished, using the
    // memory address of this instance as the uniquely identifying tag for
    // the event.
    status_ = FINISH;
    responder_.Finish(reply_, status, this);
  }

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

  // The means to get back to the client.
  ::grpc::ServerAsyncResponseWriter<ReplyType> responder_;

  // Let's implement a tiny state machine with the following states.
  enum CallStatus { CREATE, PROCESS, WAIT, FINISH };
  CallStatus status_;  // The current serving state.
};

class CallDataModelInfer : public CallData<inference::ModelInferRequest,
                                           inference::ModelInferResponse> {
 public:
  CallDataModelInfer(AsyncService* service, ::grpc::ServerCompletionQueue* cq);

  const inference::ModelInferRequest& getRequest() const;
  inference::ModelInferResponse& getReply();

 protected:
  void addNewCallData() override;
  void waitForRequest() override;
  void handleRequest() override;
};

namespace grpc {

void start(const std::string& address);
void stop();

}  // namespace grpc

}  // namespace proteus

#endif  // PROTEUS_ENABLE_GRPC

#endif  // GUARD_PROTEUS_SERVERS_GRPC_SERVER
