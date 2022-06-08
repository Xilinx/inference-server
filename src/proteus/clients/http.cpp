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

#include <drogon/HttpClient.h>            // for HttpClient, HttpClientPtr
#include <drogon/HttpRequest.h>           // for HttpRequest
#include <drogon/HttpResponse.h>          // for HttpResponse
#include <drogon/HttpTypes.h>             // for Get, ReqResult, k200OK
#include <json/value.h>                   // for Value, arrayValue, obje...
#include <trantor/net/EventLoop.h>        // for EventLoop
#include <trantor/net/EventLoopThread.h>  // for EventLoopThread

#include <cassert>      // for assert
#include <set>          // for set
#include <stdexcept>    // for invalid_argument, runti...
#include <string_view>  // for string_view
#include <thread>       // for thread
#include <utility>      // for tuple_element<>::type

#include "proteus/build_options.hpp"          // for PROTEUS_ENABLE_HTTP
#include "proteus/clients/http_internal.hpp"  // for mapJsonToResponse, mapP...
#include "proteus/servers/http_server.hpp"    // for stop, start

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
  explicit HttpClientImpl(const std::string& address,
                          const StringMap& headers) {
    loop_.run();
    client_ = drogon::HttpClient::newHttpClient(address, loop_.getLoop());
    headers_ = headers;
  }

  drogon::HttpClient* getClient() { return client_.get(); }

  void addHeaders(drogon::HttpRequestPtr req) const {
    for (const auto& [field, value] : headers_) {
      req->addHeader(field, value);
    }
  }

 private:
  trantor::EventLoopThread loop_;
  drogon::HttpClientPtr client_;
  StringMap headers_;
};

HttpClient::HttpClient(const std::string& address, const StringMap& headers) {
  this->impl_ = std::make_unique<HttpClient::HttpClientImpl>(address, headers);
}

// needed for HttpClientImpl forward declaration in WebSocket client
HttpClient::~HttpClient() = default;

void check_error(drogon::ReqResult result) {
  using drogon::ReqResult;

  std::string error_msg;
  switch (result) {
    case ReqResult::Ok:
      break;
    case ReqResult::BadServerAddress:
      error_msg = "Cannot connect to the server";
      break;
    default:
      error_msg =
        "Request error code: " + std::to_string(static_cast<int>(result));
  }
  if (!error_msg.empty()) {
    throw std::runtime_error(error_msg);
  }
}

ServerMetadata HttpClient::serverMetadata() {
  auto* client = this->impl_->getClient();

  auto req = drogon::HttpRequest::newHttpRequest();
  req->setMethod(drogon::Get);
  req->setPath("/v2");
  impl_->addHeaders(req);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  if (response->getStatusCode() != drogon::k200OK) {
    throw std::invalid_argument(response->getJsonError());
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
  auto* client = this->impl_->getClient();

  auto req = drogon::HttpRequest::newHttpRequest();
  req->setMethod(drogon::Get);
  auto path = "/v2/health/live";
  req->setPath(path);
  impl_->addHeaders(req);

  auto [result, response] = client->sendRequest(req);
  if (result != drogon::ReqResult::Ok) {
    return false;
  }
  return response->statusCode() == drogon::k200OK;
}

bool HttpClient::serverReady() {
  auto* client = this->impl_->getClient();

  auto req = drogon::HttpRequest::newHttpRequest();
  req->setMethod(drogon::Get);
  auto path = "/v2/health/ready";
  req->setPath(path);
  impl_->addHeaders(req);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  return response->statusCode() == drogon::k200OK;
}

bool HttpClient::modelReady(const std::string& model) {
  auto* client = this->impl_->getClient();

  auto req = drogon::HttpRequest::newHttpRequest();
  req->setMethod(drogon::Get);
  auto path = "/v2/models/" + model + "/ready";
  req->setPath(path);
  impl_->addHeaders(req);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  if (response->statusCode() == drogon::k400BadRequest) {
    throw std::invalid_argument(response->body().data());
  }
  return response->statusCode() == drogon::k200OK;
}

void HttpClient::modelLoad(const std::string& model,
                           RequestParameters* parameters) {
  auto* client = this->impl_->getClient();

  Json::Value json = Json::objectValue;
  if (parameters != nullptr) {
    json = mapParametersToJson(parameters);
  }

  auto req = drogon::HttpRequest::newHttpJsonRequest(json);
  req->setMethod(drogon::Post);
  auto path = "/v2/repository/models/" + model + "/load";
  req->setPath(path);
  impl_->addHeaders(req);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  if (response->statusCode() != drogon::k200OK) {
    throw std::invalid_argument(std::string(response->body()));
  }
}

void HttpClient::modelUnload(const std::string& model) {
  auto* client = this->impl_->getClient();

  Json::Value json;
  auto req = drogon::HttpRequest::newHttpJsonRequest(json);
  req->setMethod(drogon::Post);
  auto path = "/v2/repository/models/" + model + "/unload";
  req->setPath(path);
  impl_->addHeaders(req);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  auto status = response->statusCode();
  if (status != drogon::k200OK) {
    throw std::invalid_argument("Status: " +
                                std::to_string(static_cast<int>(status)));
  }
}

std::string HttpClient::workerLoad(const std::string& model,
                                   RequestParameters* parameters) {
  auto* client = this->impl_->getClient();

  Json::Value json = Json::objectValue;
  if (parameters != nullptr) {
    json = mapParametersToJson(parameters);
  }

  auto req = drogon::HttpRequest::newHttpJsonRequest(json);
  req->setMethod(drogon::Post);
  auto path = "/v2/repository/models/" + model + "/load";
  req->setPath(path);
  impl_->addHeaders(req);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  if (response->statusCode() == drogon::k400BadRequest) {
    throw std::invalid_argument(std::string(response->body()));
  }
  return std::string(response->body());
}

void HttpClient::workerUnload(const std::string& model) {
  auto* client = this->impl_->getClient();

  Json::Value json;
  auto req = drogon::HttpRequest::newHttpJsonRequest(json);
  req->setMethod(drogon::Post);
  auto path = "/v2/repository/models/" + model + "/unload";
  req->setPath(path);
  impl_->addHeaders(req);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  auto status = response->statusCode();
  if (status != drogon::k200OK) {
    throw std::invalid_argument("Status: " +
                                std::to_string(static_cast<int>(status)));
  }
}

InferenceResponse HttpClient::modelInfer(const std::string& model,
                                         const InferenceRequest& request) {
  auto* client = this->impl_->getClient();

  assert(!request.getInputs().empty());

  auto json = mapRequestToJson(request);
  auto req = drogon::HttpRequest::newHttpJsonRequest(json);
  req->setMethod(drogon::Post);
  auto path = "/v2/models/" + model + "/infer";
  req->setPath(path);
  impl_->addHeaders(req);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  if (response->statusCode() == drogon::k400BadRequest) {
    throw std::invalid_argument(std::string(response->body()));
  }

  auto resp = response->jsonObject();
  return mapJsonToResponse(resp.get());
}

std::vector<std::string> HttpClient::modelList() {
  auto* client = this->impl_->getClient();

  auto req = drogon::HttpRequest::newHttpRequest();
  req->setMethod(drogon::Get);
  const std::string path = "/v2/models";
  req->setPath(path);
  impl_->addHeaders(req);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  if (response->getStatusCode() != drogon::k200OK) {
    throw std::invalid_argument(response->getJsonError());
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
