// Copyright 2021 Xilinx Inc.
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

#include "proteus/servers/websocket_server.hpp"

#include <json/reader.h>  // for CharReader, CharReaderBui...
#include <json/value.h>   // for Value, arrayValue

#include <functional>  // for _Bind_helper<>::type, _Pl...
#include <memory>      // for allocator, shared_ptr
#include <string>      // for string, operator+, char_t...
#include <utility>     // for move

#include "proteus/batching/batcher.hpp"     // for Batcher
#include "proteus/core/manager.hpp"         // for Manager
#include "proteus/core/predict_api.hpp"     // for InferenceRequest, Callback
#include "proteus/core/worker_info.hpp"     // for WorkerInfo
#include "proteus/observation/tracing.hpp"  // for startSpan, Span

using drogon::HttpRequestPtr;
using drogon::WebSocketConnectionPtr;
using drogon::WebSocketMessageType;

namespace proteus::http {

WebsocketServer::WebsocketServer() {
#ifdef PROTEUS_ENABLE_LOGGING
  this->logger_ = getLogger();
#endif
  SPDLOG_LOGGER_INFO(this->logger_, "Constructed WebsocketServer");
}

void WebsocketServer::handleNewMessage(const WebSocketConnectionPtr &conn,
                                       std::string &&message,
                                       const WebSocketMessageType &type) {
  (void)type;  // suppress unused variable warning
#ifdef PROTEUS_ENABLE_TRACING
  auto trace = startTrace(__func__);
  trace->startSpan("websocket_handler");
#endif

  auto json = std::make_shared<Json::Value>();
  std::string errors;
  Json::CharReaderBuilder builder;
  Json::CharReader *reader = builder.newCharReader();
  bool parsingSuccessful = reader->parse(
    message.data(), message.data() + message.size(), json.get(), &errors);
  delete reader;  // NOLINT(cppcoreguidelines-owning-memory)

  // if we fail to get the JSON object, return
  if (!parsingSuccessful) {
    SPDLOG_LOGGER_INFO(this->logger_,
                       "Failed to parse JSON request to websocket");
    conn->shutdown(drogon::CloseCode::kInvalidMessage,
                   "No JSON could be parsed in the request");
    return;
  }

  std::string model;
  if (json->isMember("model")) {
    model = json->get("model", "").asString();
  } else {
    SPDLOG_LOGGER_INFO(this->logger_, "No model request found in websocket");
    conn->shutdown(drogon::CloseCode::kInvalidMessage,
                   "No model found in request");
    return;
  }

  auto request = std::make_unique<DrogonWs>(conn, std::move(json));

  WorkerInfo *worker = nullptr;
  try {
    worker = Manager::getInstance().getWorker(model);
  } catch (const std::invalid_argument &e) {
    SPDLOG_LOGGER_INFO(this->logger_, e.what());
    conn->shutdown(drogon::CloseCode::kInvalidMessage,
                   "Model " + model + " not loaded");
    return;
  }
  auto *batcher = worker->getBatcher();
#ifdef PROTEUS_ENABLE_TRACING
  trace->endSpan();
  request->setTrace(std::move(trace));
#endif
  batcher->enqueue(std::move(request));
}

void WebsocketServer::handleConnectionClosed(
  const WebSocketConnectionPtr &conn) {
  SPDLOG_LOGGER_INFO(this->logger_, "Websocket closed");
  (void)conn;  // suppress unused variable warning
}

void WebsocketServer::handleNewConnection(const HttpRequestPtr &req,
                                          const WebSocketConnectionPtr &conn) {
  SPDLOG_LOGGER_INFO(this->logger_, "New websocket connection");
  (void)conn;  // suppress unused variable warning
  (void)req;   // suppress unused variable warning
}

DrogonWs::DrogonWs(const drogon::WebSocketConnectionPtr &conn,
                   std::shared_ptr<Json::Value> json) {
  this->conn_ = conn;
  this->type_ = InterfaceType::kDrogonWs;
  this->json_ = std::move(json);
}

size_t DrogonWs::getInputSize() {
  auto inputs = this->json_->get("inputs", Json::arrayValue);
  if (!inputs.isArray()) {
    throw std::invalid_argument("'inputs' is not an array");
  }
  return inputs.size();
}

void drogonCallback(const drogon::WebSocketConnectionPtr &conn,
                    const InferenceResponse &response) {
  auto outputs = response.getOutputs();
  auto *msg = static_cast<std::string *>(outputs[0].getData());
  if (conn->connected()) {
    conn->send(*msg);
  }
}

std::shared_ptr<InferenceRequest> DrogonWs::getRequest(
  size_t &buffer_index, std::vector<BufferRawPtrs> input_buffers,
  std::vector<size_t> &input_offset, std::vector<BufferRawPtrs> output_buffers,
  std::vector<size_t> &output_offset, const size_t &batch_size,
  size_t &batch_offset) {
  std::shared_ptr<InferenceRequest> request;
  try {
    auto request = std::make_shared<InferenceRequest>(
      this->json_, buffer_index, input_buffers, input_offset, output_buffers,
      output_offset, batch_size, batch_offset);
    Callback callback =
      std::bind(drogonCallback, this->conn_, std::placeholders::_1);
    request->setCallback(std::move(callback));
    return request;
  } catch (const std::invalid_argument &e) {
    SPDLOG_LOGGER_INFO(this->logger_, e.what());
    this->conn_->shutdown(drogon::CloseCode::kUnexpectedCondition,
                          "Failed to create request");
    return nullptr;
  }
}

void DrogonWs::errorHandler(const std::invalid_argument &e) {
  SPDLOG_LOGGER_DEBUG(this->logger_, e.what());
  this->conn_->shutdown(drogon::CloseCode::kUnexpectedCondition, e.what());
}

}  // namespace proteus::http
