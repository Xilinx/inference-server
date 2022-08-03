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
 * @brief Implements the HTTP REST server in Proteus
 */

#include "proteus/servers/http_server.hpp"

#include <drogon/HttpAppFramework.h>  // for HttpAppFramework, app
#include <drogon/HttpRequest.h>       // for HttpRequestPtr, Htt...
#include <json/value.h>               // for Value, arrayValue
#include <trantor/utils/Logger.h>     // for Logger, Logger::kWarn

#include <algorithm>  // for transform
#include <chrono>     // for high_resolution_clock
#include <exception>  // for exception
#include <memory>     // for allocator, shared_ptr
#include <set>        // for set
#include <stdexcept>  // for invalid_argument
#include <string>     // for operator+, string
#include <utility>    // for move
#include <vector>     // for vector

#include "proteus/batching/batcher.hpp"           // for Batcher
#include "proteus/build_options.hpp"              // for PROTEUS_ENABLE_TRACING
#include "proteus/clients/http_internal.hpp"      // for propagate, DrogonHttp
#include "proteus/clients/native.hpp"             // for getHardware
#include "proteus/core/api.hpp"                   // for modelLoad
#include "proteus/core/model_repository.hpp"      // for loadModel
#include "proteus/core/predict_api_internal.hpp"  // for RequestParametersPtr
#include "proteus/core/worker_info.hpp"           // for WorkerInfo
#include "proteus/helpers/string.hpp"             // for toLower
#include "proteus/observation/logging.hpp"        // for Logger
#include "proteus/observation/metrics.hpp"        // for Metrics, MetricCoun...
#include "proteus/observation/tracing.hpp"        // for startTrace, Trace
#include "proteus/servers/server.hpp"             // for getLogDirectory
#include "proteus/servers/websocket_server.hpp"   // for WebsocketServer

using drogon::HttpRequestPtr;
using drogon::HttpResponse;
using drogon::HttpResponsePtr;
using drogon::HttpStatusCode;

namespace proteus::http {

void start(int port) {
  auto controller = std::make_shared<v2::ProteusHttpServer>();
  auto ws_controller = std::make_shared<WebsocketServer>();

  auto &app = drogon::app();
  app.registerController(controller);
  app.registerController(ws_controller);
#ifdef PROTEUS_ENABLE_LOGGING
  auto dir = getLogDirectory();
  app.setLogLevel(trantor::Logger::kWarn).setLogPath(dir);
#else
  app.setLogLevel(trantor::Logger::kFatal).setLogPath(".");
#endif

  app.addListener("0.0.0.0", port)
    .setThreadNum(kDefaultDrogonThreads)
    .setDocumentRoot("./src/gui/build")
    .registerPostHandlingAdvice([](const drogon::HttpRequestPtr &req,
                                   const drogon::HttpResponsePtr &resp) {
      (void)req;  // suppress unused variable warning
      resp->addHeader("Access-Control-Allow-Origin", "*");
    })
    .setClientMaxBodySize(kMaxClientBodySize)
    // .enableRunAsDaemon()
    .run();
}

void stop() { drogon::app().quit(); }

v2::ProteusHttpServer::ProteusHttpServer() {
  PROTEUS_LOG_DEBUG(logger_, "Constructed v2::ProteusHttpServer");
}

#ifdef PROTEUS_ENABLE_REST

void v2::ProteusHttpServer::getServerLive(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback) const {
  PROTEUS_LOG_INFO(logger_, "Received getServerLive request");
#ifdef PROTEUS_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::kRestGet);
#endif
#ifdef PROTEUS_ENABLE_TRACING
  auto trace = startTrace(&(__func__[0]), req->getHeaders());
#else
  (void)req;  // suppress unused variable warning
#endif

  auto resp = HttpResponse::newHttpResponse();
#ifdef PROTEUS_ENABLE_TRACING
  auto context = trace->propagate();
  propagate(resp.get(), context);
#endif
  callback(resp);
}

void v2::ProteusHttpServer::getServerReady(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback) const {
  PROTEUS_LOG_INFO(logger_, "Received getServerReady request");
#ifdef PROTEUS_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::kRestGet);
#endif
  (void)req;  // suppress unused variable warning

  // for now, assuming that server is always ready (assumes that user has loaded
  // all the required models).

  auto resp = HttpResponse::newHttpResponse();
  callback(resp);
}

void v2::ProteusHttpServer::getModelReady(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback,
  std::string const &model) const {
  PROTEUS_LOG_INFO(logger_, "Received getModelReady request");
#ifdef PROTEUS_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::kRestGet);
#endif
  (void)req;  // suppress unused variable warning

  auto resp = HttpResponse::newHttpResponse();
  try {
    if (!::proteus::modelReady(model)) {
      resp->setStatusCode(HttpStatusCode::k503ServiceUnavailable);
    }
  } catch (const invalid_argument &e) {
    resp->setStatusCode(HttpStatusCode::k400BadRequest);
    resp->setBody(e.what());
  }
  callback(resp);
}

void v2::ProteusHttpServer::getServerMetadata(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback) const {
  PROTEUS_LOG_INFO(logger_, "Received getServerMetadata request");
#ifdef PROTEUS_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::kRestGet);
#endif
  (void)req;  // suppress unused variable warning

  auto metadata = serverMetadata();

  Json::Value ret;
  ret["name"] = metadata.name;
  ret["version"] = metadata.version;
  ret["extensions"] = Json::arrayValue;
  for (const auto &extension : metadata.extensions) {
    ret["extensions"].append(extension);
  }
  auto resp = HttpResponse::newHttpJsonResponse(ret);
  callback(resp);
}

void v2::ProteusHttpServer::getModelMetadata(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback,
  const std::string &model) const {
  PROTEUS_LOG_INFO(logger_, "Received getModelMetadata request");
#ifdef PROTEUS_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::kRestGet);
#endif
  (void)req;  // suppress unused variable warning

  Json::Value ret;
  bool error = false;
  try {
    auto metadata = ::proteus::modelMetadata(model);
    ret = ModelMetadataToJson(metadata);
  } catch (const runtime_error &e) {
    ret["error"] = e.what();
    error = true;
  }

  auto resp = HttpResponse::newHttpJsonResponse(ret);
  if (error) {
    resp->setStatusCode(HttpStatusCode::k400BadRequest);
  }
  callback(resp);
}

void v2::ProteusHttpServer::modelList(
  const drogon::HttpRequestPtr &req,
  std::function<void(const drogon::HttpResponsePtr &)> &&callback) const {
  (void)req;  // suppress unused variable warning
  const auto models = ::proteus::modelList();

  Json::Value json;
  json["models"] = Json::arrayValue;
  for (const auto &model : models) {
    json["models"].append(model);
  }

  auto resp = HttpResponse::newHttpJsonResponse(json);
  callback(resp);
}

void v2::ProteusHttpServer::hasHardware(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback) const {
  PROTEUS_LOG_INFO(logger_, "Received hasHardware request");
#ifdef PROTEUS_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::kRestGet);
#endif
  auto &json = req->jsonObject();

  if (json == nullptr) {
    auto resp = errorHttpResponse("No JSON body in hasHardware request",
                                  HttpStatusCode::k400BadRequest);
    callback(resp);
    return;
  }

  auto found = proteus::hasHardware(json->get("name", "").asString(),
                                    json->get("num", 1).asInt());

  auto resp = HttpResponse::newHttpResponse();
  if (!found) {
    resp->setStatusCode(drogon::k404NotFound);
  }
  callback(resp);
}

void v2::ProteusHttpServer::modelInfer(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback,
  std::string const &model) const {
#ifdef PROTEUS_ENABLE_TRACING
  auto trace = startTrace(&(__func__[0]), req->getHeaders());
  trace->setAttribute("model", model);
#endif

  PROTEUS_LOG_INFO(logger_, "Received modelInfer request for " + model);
#ifdef PROTEUS_ENABLE_METRICS
  auto now = std::chrono::high_resolution_clock::now();
  Metrics::getInstance().incrementCounter(MetricCounterIDs::kRestPost);
#endif

#ifdef PROTEUS_ENABLE_TRACING
  trace->startSpan("request_handler");
#endif

  try {
    auto request = std::make_unique<DrogonHttp>(req, std::move(callback));
#ifdef PROTEUS_ENABLE_METRICS
    request->set_time(now);
#endif
#ifdef PROTEUS_ENABLE_TRACING
    trace->endSpan();
    request->setTrace(std::move(trace));
#endif
    ::proteus::modelInfer(model, std::move(request));
  } catch (const invalid_argument &e) {
    PROTEUS_LOG_INFO(logger_, e.what());
    auto resp = errorHttpResponse(e.what(), HttpStatusCode::k400BadRequest);
#ifdef PROTEUS_ENABLE_TRACING
    auto context = trace->propagate();
    propagate(resp.get(), context);
#endif
    callback(resp);
  }
}

void v2::ProteusHttpServer::modelLoad(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback,
  const std::string &model) const {
  auto model_lower = toLower(model);
#ifdef PROTEUS_ENABLE_TRACING
  auto trace = startTrace(&(__func__[0]), req->getHeaders());
  trace->setAttribute("model", model_lower);
#endif
  PROTEUS_LOG_INFO(logger_, "Received modelLoad request for " + model_lower);

  auto json = req->getJsonObject();
  RequestParametersPtr parameters = nullptr;
  if (json != nullptr) {
    parameters = mapJsonToParameters(*json);
  } else {
    parameters = std::make_unique<RequestParameters>();
  }
#ifdef PROTEUS_ENABLE_TRACING
  trace->setAttributes(parameters.get());
#endif

  try {
    ::proteus::modelLoad(model_lower, parameters.get());
  } catch (const runtime_error &e) {
    PROTEUS_LOG_ERROR(logger_, e.what());
    auto resp = errorHttpResponse(e.what(), HttpStatusCode::k400BadRequest);
#ifdef PROTEUS_ENABLE_TRACING
    auto context = trace->propagate();
    propagate(resp.get(), context);
#endif
    callback(resp);
  }

  auto resp = HttpResponse::newHttpResponse();
#ifdef PROTEUS_ENABLE_TRACING
  auto context = trace->propagate();
  propagate(resp.get(), context);
#endif
  callback(resp);
}

void v2::ProteusHttpServer::modelUnload(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback,
  const std::string &model) const {
  PROTEUS_LOG_INFO(logger_, "Received modelUnload request");
#ifdef PROTEUS_ENABLE_TRACING
  auto trace = startTrace(&(__func__[0]), req->getHeaders());
#endif

  auto model_lower = toLower(model);

#ifdef PROTEUS_ENABLE_TRACING
  trace->setAttribute("model", model_lower);
#endif

  ::proteus::modelUnload(model_lower);

  auto resp = HttpResponse::newHttpResponse();
#ifdef PROTEUS_ENABLE_TRACING
  auto context = trace->propagate();
  propagate(resp.get(), context);
#endif
  callback(resp);
}

void v2::ProteusHttpServer::workerLoad(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback,
  const std::string &worker) const {
  PROTEUS_LOG_INFO(logger_, "Received load request");
#ifdef PROTEUS_ENABLE_TRACING
  auto trace = startTrace(&(__func__[0]), req->getHeaders());
#endif

  auto json = req->getJsonObject();
  RequestParametersPtr parameters = nullptr;
  if (json != nullptr) {
    parameters = mapJsonToParameters(*json);
  } else {
    parameters = std::make_unique<RequestParameters>();
  }

  auto worker_lower = toLower(worker);

#ifdef PROTEUS_ENABLE_TRACING
  trace->setAttribute("model", worker_lower);
#endif
  PROTEUS_LOG_INFO(logger_, "Received load request is for " + worker_lower);

#ifdef PROTEUS_ENABLE_TRACING
  trace->setAttributes(parameters.get());
#endif
  HttpResponsePtr resp;
  try {
    auto endpoint = ::proteus::workerLoad(worker_lower, parameters.get());
    resp = HttpResponse::newHttpResponse();
    resp->setBody(endpoint);
  } catch (const runtime_error &e) {
    PROTEUS_LOG_ERROR(logger_, e.what());
    resp = errorHttpResponse(e.what(), HttpStatusCode::k400BadRequest);
  }

#ifdef PROTEUS_ENABLE_TRACING
  auto context = trace->propagate();
  propagate(resp.get(), context);
#endif
  callback(resp);
}

void v2::ProteusHttpServer::workerUnload(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback,
  const std::string &worker) const {
#ifdef PROTEUS_ENABLE_TRACING
  auto trace = startTrace(&(__func__[0]), req->getHeaders());
#endif

  auto worker_lower = toLower(worker);

  PROTEUS_LOG_INFO(logger_, "Received unload request is for " + worker_lower);

#ifdef PROTEUS_ENABLE_TRACING
  trace->setAttribute("model", worker_lower);
#endif

  ::proteus::workerUnload(worker_lower);

  auto resp = HttpResponse::newHttpResponse();
#ifdef PROTEUS_ENABLE_TRACING
  auto context = trace->propagate();
  propagate(resp.get(), context);
#endif
  callback(resp);
}

#endif  // PROTEUS_ENABLE_REST

#ifdef PROTEUS_ENABLE_METRICS
void v2::ProteusHttpServer::metrics(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback) const {
  (void)req;  // suppress unused variable warning
  PROTEUS_LOG_INFO(logger_, "Received metrics request");
  std::string body = Metrics::getInstance().getMetrics();
  auto resp = drogon::HttpResponse::newHttpResponse();
  resp->setBody(body);
  resp->setContentTypeCode(drogon::ContentType::CT_TEXT_PLAIN);
  callback(resp);
}
#endif

}  // namespace proteus::http
