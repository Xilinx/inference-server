// Copyright 2021 Xilinx Inc.
// Copyright 2022 Advanced Micro Devices Inc.
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

#include <drogon/HttpAppFramework.h>      // for HttpAppFramework, app
#include <drogon/HttpClient.h>            // for HttpClient, HttpClientPtr
#include <drogon/HttpRequest.h>           // for HttpRequest, HttpReques...
#include <drogon/HttpResponse.h>          // for HttpResponse
#include <drogon/HttpTypes.h>             // for k200OK, Get, Post, ReqR...
#include <json/value.h>                   // for Value, arrayValue, obje...
#include <trantor/net/EventLoopThread.h>  // for EventLoopThread

#include <atomic>
#include <cassert>        // for assert
#include <unordered_set>  // for unordered_set
#include <utility>        // for tuple_element<>::type
#include <vector>

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
  explicit HttpClientImpl(const std::string& address, int parallelism)
    : num_clients_(parallelism) {
    // arbitrarily use ratio of 16:1 between HttpClients and EventLoops
    const auto kClientThreadRatio = 16;
    const auto threads = (parallelism / kClientThreadRatio) + 1;

    loops_.reserve(threads);
    clients_.reserve(num_clients_);
    for (auto i = 0; i < threads; ++i) {
      // need to use unique_ptr because EventLoopThreads are not moveable or
      // copyable and so incompatible with std::vectors
      const auto& loop =
        loops_.emplace_back(std::make_unique<trantor::EventLoopThread>());
      loop->run();
    }
    for (auto i = 0; i < num_clients_; ++i) {
      const auto& loop = loops_[i % threads];
      clients_.emplace_back(
        drogon::HttpClient::newHttpClient(address, loop->getLoop()));
    }
  }

  drogon::HttpClient* getClient() {
    const auto& client = clients_[counter_];
    counter_ = (counter_ + 1) % num_clients_;
    return client.get();
  }

  auto getClientNum() const { return num_clients_; }

 private:
  int counter_ = 0;
  int num_clients_;
  std::vector<std::unique_ptr<trantor::EventLoopThread>> loops_;
  std::vector<drogon::HttpClientPtr> clients_;
};

HttpClient::HttpClient(std::string address, const StringMap& headers,
                       int parallelism)
  : address_(std::move(address)), headers_(headers) {
  this->impl_ =
    std::make_unique<HttpClient::HttpClientImpl>(address_, parallelism);
}

HttpClient::HttpClient(const HttpClient& other)
  : address_(other.address_), headers_(other.headers_) {
  this->impl_ = std::make_unique<HttpClient::HttpClientImpl>(
    address_, other.impl_->getClientNum());
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

drogon::HttpRequestPtr createGetRequest(const std::string& path,
                                        const StringMap& headers) {
  auto req = drogon::HttpRequest::newHttpRequest();
  req->setMethod(drogon::Get);
  req->setPath(path);
  addHeaders(req, headers);
  return req;
}

drogon::HttpRequestPtr createPostRequest(const Json::Value& json,
                                         const std::string& path,
                                         const StringMap& headers) {
  auto req = drogon::HttpRequest::newHttpJsonRequest(json);
  req->setMethod(drogon::Post);
  req->setPath(path);
  addHeaders(req, headers);
  return req;
}

ServerMetadata HttpClient::serverMetadata() {
  auto* client = this->impl_->getClient();
  auto req = createGetRequest("/v2", headers_);

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
  auto req = createGetRequest("/v2/health/live", headers_);

  auto [result, response] = client->sendRequest(req);
  if (result != drogon::ReqResult::Ok) {
    return false;
  }
  return response->statusCode() == drogon::k200OK;
}

bool HttpClient::serverReady() {
  auto* client = this->impl_->getClient();
  auto req = createGetRequest("/v2/health/ready", headers_);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  return response->statusCode() == drogon::k200OK;
}

bool HttpClient::modelReady(const std::string& model) {
  auto* client = this->impl_->getClient();
  auto req = createGetRequest("/v2/models/" + model + "/ready", headers_);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  return response->statusCode() == drogon::k200OK;
}

ModelMetadata HttpClient::modelMetadata(const std::string& model) {
  auto* client = this->impl_->getClient();
  auto req = createGetRequest("/v2/models/" + model, headers_);

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

  auto req = createPostRequest(json, "/v2/repository/models/" + model + "/load",
                               headers_);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  if (response->statusCode() != drogon::k200OK) {
    throw bad_status(std::string(response->body()));
  }
}

void HttpClient::modelUnload(const std::string& model) {
  auto* client = this->impl_->getClient();

  Json::Value json;
  auto req = createPostRequest(
    json, "/v2/repository/models/" + model + "/unload", headers_);

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

  auto req =
    createPostRequest(json, "/v2/workers/" + model + "/load", headers_);

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
  auto req =
    createPostRequest(json, "/v2/workers/" + model + "/unload", headers_);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  auto status = response->statusCode();
  if (status != drogon::k200OK) {
    throw bad_status(std::string(response->body()));
  }
}

auto createInferenceRequest(const std::string& model,
                            const InferenceRequest& request,
                            const StringMap& headers) {
  assert(!request.getInputs().empty());

  auto json = mapRequestToJson(request);
  return createPostRequest(json, "/v2/models/" + model + "/infer", headers);
}

InferenceResponseFuture HttpClient::modelInferAsync(
  const std::string& model, const InferenceRequest& request) {
  auto req = createInferenceRequest(model, request, headers_);
  auto prom = std::make_shared<std::promise<proteus::InferenceResponse>>();
  auto fut = prom->get_future();

  auto* client = this->impl_->getClient();
  client->sendRequest(
    req, [prom = std::move(prom)](drogon::ReqResult result,
                                  const drogon::HttpResponsePtr& response) {
      check_error(result);
      if (response->statusCode() != drogon::k200OK) {
        throw bad_status(std::string(response->body()));
      }

      auto resp = response->jsonObject();
      prom->set_value(mapJsonToResponse(resp.get()));
    });

  return fut;
}

InferenceResponse HttpClient::modelInfer(const std::string& model,
                                         const InferenceRequest& request) {
  auto req = createInferenceRequest(model, request, headers_);

  auto* client = this->impl_->getClient();
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
  auto req = createGetRequest("/v2/models", headers_);

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
  auto req = createPostRequest(json, "/v2/hardware", headers_);

  auto [result, response] = client->sendRequest(req);
  check_error(result);
  return response->statusCode() == drogon::k200OK;
}

const std::string& HttpClient::getAddress() const& { return address_; }
std::string HttpClient::getAddress() const&& { return address_; }

const StringMap& HttpClient::getHeaders() const& { return headers_; }
StringMap HttpClient::getHeaders() const&& { return headers_; }

}  // namespace proteus
