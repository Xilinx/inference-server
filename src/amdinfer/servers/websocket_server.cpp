// Copyright 2021 Xilinx, Inc.
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

#include "amdinfer/servers/websocket_server.hpp"

#include <json/reader.h>  // for CharReader, CharReaderBui...
#include <json/value.h>   // for Value, arrayValue

#include <algorithm>  // for transform
#include <cctype>     // for tolower
#include <memory>     // for allocator, shared_ptr
#include <string>     // for string, operator+, char_t...
#include <utility>    // for move

#include "amdinfer/core/exceptions.hpp"          // for invalid_argument
#include "amdinfer/core/inference_request.hpp"   // for InferenceRequest
#include "amdinfer/core/inference_response.hpp"  // for InferenceResponse
#include "amdinfer/core/request_container.hpp"   // for ParameterMapPtr
#include "amdinfer/core/shared_state.hpp"        // for SharedState
#include "amdinfer/observation/tracing.hpp"      // for startSpan, Span
#include "amdinfer/servers/http_server.hpp"      // for RequestBuilder

using drogon::HttpRequestPtr;
using drogon::WebSocketConnectionPtr;
using drogon::WebSocketMessageType;

namespace amdinfer::http {

void setCallback(InferenceRequest *request,
                 drogon::WebSocketConnectionPtr conn) {
  Callback callback = [conn =
                         std::move(conn)](const InferenceResponse &response) {
    if (response.isError()) {
      conn->send(response.getError());
    } else {
      auto outputs = response.getOutputs();
      const auto *msg = static_cast<char *>(outputs[0].getData());
      if (conn->connected()) {
        conn->send(msg, outputs[0].getSize());
      }
    }
  };
  request->setCallback(std::move(callback));
}

WebsocketServer::WebsocketServer(SharedState *state) : state_(state) {
  AMDINFER_LOG_INFO(logger_, "Constructed WebsocketServer");
}

void WebsocketServer::handleNewMessage(const WebSocketConnectionPtr &conn,
                                       std::string &&message,
                                       const WebSocketMessageType &type) {
#ifdef AMDINFER_ENABLE_TRACING
  auto trace = startTrace(&(__func__[0]));
  trace->startSpan("websocket_handler");
#endif

  if (type == WebSocketMessageType::Close) {
    conn->shutdown(drogon::CloseCode::kNormalClosure, "");
    return;
  }

  auto json = std::make_shared<Json::Value>();
  std::string errors;
  Json::CharReaderBuilder builder;
  Json::CharReader *reader = builder.newCharReader();
  bool parsing_successful = reader->parse(
    message.data(), message.data() + message.size(), json.get(), &errors);
  delete reader;  // NOLINT(cppcoreguidelines-owning-memory)

  // if we fail to get the JSON object, return
  if (!parsing_successful) {
    AMDINFER_LOG_INFO(logger_, "Failed to parse JSON request to websocket");
    conn->shutdown(drogon::CloseCode::kInvalidMessage,
                   "No JSON could be parsed in the request");
    return;
  }

  std::string model;
  if (json->isMember("model")) {
    model = json->get("model", "").asString();
  } else {
    AMDINFER_LOG_INFO(logger_, "No model request found in websocket");
    conn->shutdown(drogon::CloseCode::kInvalidMessage,
                   "No model found in request");
    return;
  }
  std::transform(model.begin(), model.end(), model.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  auto request = getRequest(json, state_->getPool());
  setCallback(request.get(), conn);
  auto request_container = std::make_unique<RequestContainer>();
  request_container->request = request;
#ifdef AMDINFER_ENABLE_TRACING
  trace->endSpan();
  request_container->trace = std::move(trace);
#endif

  try {
    state_->modelInfer(model, std::move(request_container));
  } catch (const runtime_error &e) {
    AMDINFER_LOG_INFO(logger_, e.what());
    conn->shutdown(drogon::CloseCode::kInvalidMessage, e.what());
    return;
  }
}

void WebsocketServer::handleConnectionClosed(
  const WebSocketConnectionPtr &conn) {
  AMDINFER_LOG_INFO(logger_, "Websocket closed");
  // (void)conn;  // suppress unused variable warning
  conn->shutdown();
}

void WebsocketServer::handleNewConnection(const HttpRequestPtr &req,
                                          const WebSocketConnectionPtr &conn) {
  AMDINFER_LOG_INFO(logger_, "New websocket connection");
  (void)conn;  // suppress unused variable warning
  (void)req;   // suppress unused variable warning
}

}  // namespace amdinfer::http
