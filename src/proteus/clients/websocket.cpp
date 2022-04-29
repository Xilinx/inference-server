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
 * @brief Implements the methods for interacting with Proteus with WebSocket
 */

#include "proteus/clients/websocket.hpp"

#include <concurrentqueue/blockingconcurrentqueue.h>
#include <drogon/HttpClient.h>
#include <drogon/WebSocketClient.h>
#include <trantor/net/EventLoopThread.h>

#include <thread>

#include "proteus/build_options.hpp"
#include "proteus/clients/http.hpp"
#include "proteus/clients/http_internal.hpp"
// #include "proteus/servers/http_server.hpp"

namespace proteus {

class WebSocketClient::WebSocketClientImpl {
 public:
  WebSocketClientImpl(const std::string& ws_address,
                      const std::string& http_address) {
    using drogon::WebSocketMessageType;

    loop_.run();
    ws_client_ =
      drogon::WebSocketClient::newWebSocketClient(ws_address, loop_.getLoop());
    http_client_ = std::make_unique<HttpClient>(http_address);

    ws_client_->setMessageHandler(
      [&](const std::string& message, const drogon::WebSocketClientPtr&,
          const drogon::WebSocketMessageType& type) {
        std::string messageType = "Unknown";
        switch (type) {
          case WebSocketMessageType::Text: {
            Json::CharReaderBuilder builder;
            Json::CharReader* reader = builder.newCharReader();

            Json::Value root;
            std::string errors;

            bool parsingSuccessful =
              reader->parse(message.c_str(), message.c_str() + message.size(),
                            &root, &errors);
            delete reader;
            if (!parsingSuccessful) {
              throw std::runtime_error("Unsuccessful?");
            }
            auto json_ptr = std::make_shared<Json::Value>(std::move(root));
            queue_.enqueue(mapJsonToResponse(json_ptr));
          }
          default: {
            break;
          }
        }
      });
  }

  ~WebSocketClientImpl() { loop_.getLoop()->quit(); }

  void connect() {
    auto connection = ws_client_->getConnection();
    if (connection->disconnected()) {
      this->connected_ = false;
      auto req = drogon::HttpRequest::newHttpRequest();
      req->setMethod(drogon::Get);
      req->setPath("/models/infer");
      ws_client_->connectToServer(req, [&](drogon::ReqResult r,
                                           const drogon::HttpResponsePtr&,
                                           const drogon::WebSocketClientPtr&) {
        if (r != drogon::ReqResult::Ok) {
          throw std::runtime_error("Failed to establish WebSocket connection!");
        }
        this->connected_ = true;
      });
    }
    while (!connected_) {
      std::this_thread::yield();
    }
  }

  InferenceResponse recv() {
    InferenceResponse response;
    queue_.wait_dequeue(response);
    return response;
  }

  drogon::WebSocketClient* getWsClient() { return ws_client_.get(); }
  HttpClient* getHttpClient() { return http_client_.get(); }

 private:
  trantor::EventLoopThread loop_;
  std::unique_ptr<HttpClient> http_client_;
  drogon::WebSocketClientPtr ws_client_;
  bool connected_ = false;
  moodycamel::BlockingConcurrentQueue<InferenceResponse> queue_;
};

WebSocketClient::WebSocketClient(const std::string& ws_address,
                                 const std::string& http_address) {
  this->impl_ = std::make_unique<WebSocketClient::WebSocketClientImpl>(
    ws_address, http_address);
}

WebSocketClient::~WebSocketClient() = default;

ServerMetadata WebSocketClient::serverMetadata() {
  auto client = this->impl_->getHttpClient();
  return client->serverMetadata();
}

bool WebSocketClient::serverLive() {
  auto client = this->impl_->getHttpClient();
  return client->serverLive();
}

bool WebSocketClient::serverReady() {
  auto client = this->impl_->getHttpClient();
  return client->serverReady();
}

bool WebSocketClient::modelReady(const std::string& model) {
  auto client = this->impl_->getHttpClient();
  return client->modelReady(model);
}

std::string WebSocketClient::modelLoad(const std::string& model,
                                       RequestParameters* parameters) {
  auto client = this->impl_->getHttpClient();
  return client->modelLoad(model, parameters);
}

void WebSocketClient::modelUnload(const std::string& model) {
  auto client = this->impl_->getHttpClient();
  client->modelUnload(model);
}

InferenceResponse WebSocketClient::modelInfer(const std::string& model,
                                              const InferenceRequest& request) {
  auto client = this->impl_->getHttpClient();
  return client->modelInfer(model, request);
}

std::vector<std::string> WebSocketClient::modelList() {
  auto client = this->impl_->getHttpClient();
  return client->modelList();
}

void WebSocketClient::modelInferAsync(const std::string& model,
                                      const InferenceRequest& request) {
  auto client = this->impl_->getWsClient();
  auto connection = client->getConnection();

  if (connection->disconnected()) {
    impl_->connect();
  }

  auto json = mapRequestToJson(request);
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";  // remove whitespace
  const std::string message = Json::writeString(builder, json);
  connection->send(message);
}

InferenceResponse WebSocketClient::modelRecv() { return impl_->recv(); }

}  // namespace proteus
