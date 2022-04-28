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

/**
 * @file
 * @brief Implements the methods for interacting with Proteus with HTTP/REST
 */

#include "proteus/clients/http.hpp"

#include <drogon/HttpClient.h>
#include <trantor/net/EventLoopThread.h>

#include <thread>

#include "proteus/build_options.hpp"
#include "proteus/clients/http_internal.hpp"
#include "proteus/servers/http_server.hpp"

namespace proteus {

void startHttpServer(int port) {
#ifdef PROTEUS_ENABLE_HTTP
  std::thread{http::start, port}.detach();
#else
  (void)port;  // suppress unused variable warning
#endif
  HttpClient client("http://127.0.0.1:" + std::to_string(port));
  bool ready = false;
  do {
    ready = client.serverLive();
  } while (!ready);
}

void stopHttpServer() {
#ifdef PROTEUS_ENABLE_HTTP
  http::stop();
#endif
}

class HttpClient::HttpClientImpl {
 public:
  explicit HttpClientImpl(const std::string& address) {
    loop_.run();
    client_ = drogon::HttpClient::newHttpClient(address, loop_.getLoop());
  }

  ~HttpClientImpl() { loop_.getLoop()->quit(); }

  drogon::HttpClient* getClient() { return client_.get(); }

 private:
  trantor::EventLoopThread loop_;
  drogon::HttpClientPtr client_;
};

HttpClient::HttpClient(const std::string& address) {
  this->impl_ = std::make_unique<HttpClient::HttpClientImpl>(address);
}

HttpClient::~HttpClient() = default;

ServerMetadata HttpClient::serverMetadata() {
  auto client = this->impl_->getClient();

  auto req = drogon::HttpRequest::newHttpRequest();
  req->setMethod(drogon::Get);
  req->setPath("/v2");

  auto [result, response] = client->sendRequest(req);
  if (result != drogon::ReqResult::Ok) {
    throw std::runtime_error("Request error code: " +
                             std::to_string(static_cast<int>(result)));
  }
  ServerMetadata metadata;
  auto json = response->getJsonObject();
  metadata.name = json->get("name", "").asString();
  metadata.version = json->get("version", "").asString();
  auto extensions = json->get("extensions", Json::arrayValue);
  for (auto const& extension : extensions) {
    metadata.extensions.insert(extension.asString());
  }
  return metadata;
}

bool HttpClient::serverLive() {
  auto client = this->impl_->getClient();

  auto req = drogon::HttpRequest::newHttpRequest();
  req->setMethod(drogon::Get);
  auto path = "/v2/health/live";
  req->setPath(path);

  auto [result, response] = client->sendRequest(req);
  if (result != drogon::ReqResult::Ok) {
    // throw std::runtime_error("Request error code: " +
    // std::to_string(static_cast<int>(result)));
    return false;
  }
  return response->statusCode() == drogon::k200OK;
}
bool HttpClient::serverReady() {
  auto client = this->impl_->getClient();

  auto req = drogon::HttpRequest::newHttpRequest();
  req->setMethod(drogon::Get);
  auto path = "/v2/health/ready";
  req->setPath(path);

  auto [result, response] = client->sendRequest(req);
  if (result != drogon::ReqResult::Ok) {
    throw std::runtime_error("Request error code: " +
                             std::to_string(static_cast<int>(result)));
  }
  return response->statusCode() == drogon::k200OK;
}
bool HttpClient::modelReady(const std::string& model) {
  auto client = this->impl_->getClient();

  auto req = drogon::HttpRequest::newHttpRequest();
  req->setMethod(drogon::Get);
  auto path = "/v2/models/" + model + "/ready";
  req->setPath(path);

  auto [result, response] = client->sendRequest(req);
  if (result != drogon::ReqResult::Ok) {
    throw std::runtime_error("Request error code: " +
                             std::to_string(static_cast<int>(result)));
  }
  if (response->statusCode() == drogon::k400BadRequest) {
    throw std::invalid_argument(std::string(response->body()));
  }
  return response->statusCode() == drogon::k200OK;
}

std::string HttpClient::modelLoad(const std::string& model,
                                  RequestParameters* parameters) {
  auto client = this->impl_->getClient();

  Json::Value json = Json::objectValue;
  if (parameters != nullptr) {
    json = mapParametersToJson(parameters);
  }

  auto req = drogon::HttpRequest::newHttpJsonRequest(json);
  req->setMethod(drogon::Post);
  auto path = "/v2/repository/models/" + model + "/load";
  req->setPath(path);

  auto [result, response] = client->sendRequest(req);
  if (result != drogon::ReqResult::Ok) {
    throw std::runtime_error("Request error code: " +
                             std::to_string(static_cast<int>(result)));
  }
  if (response->statusCode() == drogon::k400BadRequest) {
    throw std::invalid_argument(std::string(response->body()));
  }
  return std::string(response->body());
}

void HttpClient::modelUnload(const std::string& model) {
  auto client = this->impl_->getClient();

  Json::Value json;
  auto req = drogon::HttpRequest::newHttpJsonRequest(json);
  req->setMethod(drogon::Post);
  auto path = "/v2/repository/models/" + model + "/unload";
  req->setPath(path);

  auto [result, response] = client->sendRequest(req);
  if (result != drogon::ReqResult::Ok) {
    throw std::runtime_error("Request error code: " +
                             std::to_string(static_cast<int>(result)));
  }
  auto status = response->statusCode();
  if (status != drogon::k200OK) {
    throw std::runtime_error("Status: " +
                             std::to_string(static_cast<int>(status)));
  }
}
InferenceResponse HttpClient::modelInfer(const std::string& model,
                                         const InferenceRequest& request) {
  auto client = this->impl_->getClient();

  auto json = mapRequestToJson(request);
  auto req = drogon::HttpRequest::newHttpJsonRequest(json);
  req->setMethod(drogon::Post);
  auto path = "/v2/models/" + model + "/infer";
  req->setPath(path);

  auto [result, response] = client->sendRequest(req);
  if (result != drogon::ReqResult::Ok) {
    throw std::runtime_error("Request error code: " +
                             std::to_string(static_cast<int>(result)));
  }
  if (response->statusCode() == drogon::k400BadRequest) {
    throw std::invalid_argument(std::string(response->body()));
  }

  auto resp = response->jsonObject();
  return mapJsonToResponse(resp);
}

std::vector<std::string> HttpClient::modelList() {
  auto client = this->impl_->getClient();

  auto req = drogon::HttpRequest::newHttpRequest();
  req->setMethod(drogon::Get);
  auto path = "/v2/models";
  req->setPath(path);

  auto [result, response] = client->sendRequest(req);
  if (result != drogon::ReqResult::Ok) {
    throw std::runtime_error("Request error code: " +
                             std::to_string(static_cast<int>(result)));
  }
  auto json = response->jsonObject();

  auto json_models = json->get("models", Json::arrayValue);
  std::vector<std::string> models;
  models.reserve(json_models.size());
  for (const auto& model : json_models) {
    models.push_back(model.asString());
  }
  return models;
}

}  // namespace proteus