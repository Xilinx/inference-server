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
#include <drogon/HttpRequest.h>           // for HttpRequest, HttpReques...
#include <drogon/HttpResponse.h>          // for HttpResponse
#include <drogon/HttpTypes.h>             // for k200OK, Get, Post, ReqR...
#include <json/value.h>                   // for Value, arrayValue, obje...
#include <trantor/net/EventLoopThread.h>  // for EventLoopThread

#include <cassert>        // for assert
#include <unordered_set>  // for unordered_set
#include <utility>        // for tuple_element<>::type

#include "proteus/clients/http_internal.hpp"  // for mapParametersToJson
#include "proteus/core/exceptions.hpp"        // for bad_status

namespace proteus {

void addHeaders(drogon::HttpRequestPtr req, const StringMap& headers) {
  for (const auto& [field, value] : headers) {
    req->addHeader(field, value);
  }
}

class HttpClient::HttpClientImpl {
 public:
  explicit HttpClientImpl(const std::string& address) {
    loop_.run();
    client_ = drogon::HttpClient::newHttpClient(address, loop_.getLoop());
  }

  drogon::HttpClient* getClient() { return client_.get(); }

 private:
  trantor::EventLoopThread loop_;
  drogon::HttpClientPtr client_;
};

HttpClient::HttpClient(std::string address, const StringMap& headers)
  : address_(std::move(address)), headers_(headers) {
  this->impl_ = std::make_unique<HttpClient::HttpClientImpl>(address_);
}

HttpClient::HttpClient(const HttpClient& other)
  : address_(other.address_), headers_(other.headers_) {
  this->impl_ = std::make_unique<HttpClient::HttpClientImpl>(address_);
}

HttpClient::HttpClient(HttpClient&& other) noexcept
  : address_(std::move(other.address_)),
    headers_(std::move(other.headers_)),
    impl_(std::move(other.impl_)) {}

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
    throw bad_status(error_msg);
  }
}

ServerMetadata HttpClient::serverMetadata() {
  auto* client = this->impl_->getClient();

  auto req = drogon::HttpRequest::newHttpRequest();
  req->setMethod(drogon::Get);
  req->setPath("/v2");
  addHeaders(req, headers_);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  if (response->statusCode() != drogon::k200OK) {
    throw bad_status(response->getJsonError());
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
  addHeaders(req, headers_);

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
  addHeaders(req, headers_);

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
  addHeaders(req, headers_);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  return response->statusCode() == drogon::k200OK;
}

ModelMetadata HttpClient::modelMetadata(const std::string& model) {
  auto* client = this->impl_->getClient();
  auto req = drogon::HttpRequest::newHttpRequest();
  req->setMethod(drogon::Get);
  auto path = "/v2/models/" + model;
  req->setPath(path);
  addHeaders(req, headers_);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  auto resp = response->jsonObject();
  return mapJsonToModelMetadata(resp.get());
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
  addHeaders(req, headers_);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  if (response->statusCode() != drogon::k200OK) {
    throw bad_status(std::string(response->body()));
  }
}

void HttpClient::modelUnload(const std::string& model) {
  auto* client = this->impl_->getClient();

  Json::Value json;
  auto req = drogon::HttpRequest::newHttpJsonRequest(json);
  req->setMethod(drogon::Post);
  auto path = "/v2/repository/models/" + model + "/unload";
  req->setPath(path);
  addHeaders(req, headers_);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  auto status = response->statusCode();
  if (status != drogon::k200OK) {
    throw bad_status(std::string(response->body()));
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
  auto path = "/v2/workers/" + model + "/load";
  req->setPath(path);
  addHeaders(req, headers_);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  if (response->statusCode() != drogon::k200OK) {
    throw bad_status(std::string(response->body()));
  }
  return std::string(response->body());
}

void HttpClient::workerUnload(const std::string& model) {
  auto* client = this->impl_->getClient();

  Json::Value json;
  auto req = drogon::HttpRequest::newHttpJsonRequest(json);
  req->setMethod(drogon::Post);
  auto path = "/v2/workers/" + model + "/unload";
  req->setPath(path);
  addHeaders(req, headers_);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  auto status = response->statusCode();
  if (status != drogon::k200OK) {
    throw bad_status(std::string(response->body()));
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
  addHeaders(req, headers_);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  if (response->statusCode() != drogon::k200OK) {
    throw bad_status(std::string(response->body()));
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
  addHeaders(req, headers_);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  if (response->statusCode() != drogon::k200OK) {
    throw bad_status(response->getJsonError());
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

bool HttpClient::hasHardware(const std::string& name, int num) {
  auto* client = this->impl_->getClient();

  Json::Value json;
  json["name"] = name;
  json["num"] = num;
  auto req = drogon::HttpRequest::newHttpJsonRequest(json);
  req->setMethod(drogon::Get);
  const std::string path = "/v2/hardware";
  req->setPath(path);
  addHeaders(req, headers_);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  return response->statusCode() == drogon::k200OK;
}

const std::string& HttpClient::getAddress() const& { return address_; }
std::string HttpClient::getAddress() const&& { return address_; }

const StringMap& HttpClient::getHeaders() const& { return headers_; }
StringMap HttpClient::getHeaders() const&& { return headers_; }

}  // namespace proteus
