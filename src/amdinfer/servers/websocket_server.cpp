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

#include "amdinfer/clients/http_internal.hpp"      // for RequestBuilder
#include "amdinfer/core/exceptions.hpp"            // for invalid_argument
#include "amdinfer/core/predict_api_internal.hpp"  // for ParameterMapPtr
#include "amdinfer/core/shared_state.hpp"          // for SharedState
#include "amdinfer/observation/tracing.hpp"        // for startSpan, Span

using drogon::HttpRequestPtr;
using drogon::WebSocketConnectionPtr;
using drogon::WebSocketMessageType;

namespace amdinfer::http {

/**
 * @brief The DrogonWs ProtocolWrapper class encapsulates incoming requests from
 * Drogon's Websocket interface to the batcher.
 *
 */
class DrogonWs : public ProtocolWrapper {
 public:
  DrogonWs(const drogon::WebSocketConnectionPtr &conn,
           std::shared_ptr<Json::Value> json)
    : json_(std::move(json)), conn_(conn) {}

  std::shared_ptr<InferenceRequest> getRequest(
    const BufferRawPtrs &input_buffers, std::vector<size_t> &input_offsets,
    const BufferRawPtrs &output_buffers,
    std::vector<size_t> &output_offsets) override {
#ifdef AMDINFER_ENABLE_LOGGING
    const auto &logger = this->getLogger();
#endif
    try {
      auto request =
        RequestBuilder::build(this->json_, input_buffers, input_offsets,
                              output_buffers, output_offsets);
      Callback callback =
        [conn = std::move(this->conn_)](const InferenceResponse &response) {
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
      return request;
    } catch (const invalid_argument &e) {
      AMDINFER_LOG_INFO(logger, e.what());
      this->conn_->shutdown(drogon::CloseCode::kUnexpectedCondition,
                            "Failed to create request");
      return nullptr;
    }
  }

  size_t getInputSize() override {
    auto inputs = this->json_->get("inputs", Json::arrayValue);
    if (!inputs.isArray()) {
      throw invalid_argument("'inputs' is not an array");
    }
    return inputs.size();
  }

  std::vector<size_t> getInputSizes() const override {
    std::vector<size_t> sizes;

    if (!this->json_->isMember("inputs")) {
      throw invalid_argument("No 'inputs' key present in request");
    }
    auto inputs = this->json_->get("inputs", Json::arrayValue);
    if (!inputs.isArray()) {
      throw invalid_argument("'inputs' is not an array");
    }

    for (const auto &tensor : inputs) {
      // using asCString() doesn't work -> the string is empty
      const auto raw_type = tensor.get("datatype", "UNKNOWN").asString();
      auto datatype = DataType(raw_type.c_str());

      const auto shape = tensor.get("shape", Json::arrayValue);
      size_t size = 1;
      for (const auto &index : shape) {
        size *= index.asInt64();
      }

      sizes.push_back(size * datatype.size());
    }
    return sizes;
  }

  void errorHandler(const std::exception &e) override {
    AMDINFER_LOG_INFO(this->getLogger(), e.what());
    this->conn_->shutdown(drogon::CloseCode::kUnexpectedCondition, e.what());
  }

 private:
  std::shared_ptr<Json::Value> json_;
  drogon::WebSocketConnectionPtr conn_;
};

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

  auto request = std::make_unique<DrogonWs>(conn, std::move(json));
#ifdef AMDINFER_ENABLE_TRACING
  trace->endSpan();
  request->setTrace(std::move(trace));
#endif

  try {
    state_->modelInfer(model, std::move(request));
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
