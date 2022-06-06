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
#include "proteus/core/manager.hpp"               // for Manager
#include "proteus/core/predict_api_internal.hpp"  // for RequestParametersPtr
#include "proteus/core/worker_info.hpp"           // for WorkerInfo
#include "proteus/observation/logging.hpp"        // for Logger
#include "proteus/observation/metrics.hpp"        // for Metrics, MetricCoun...
#include "proteus/observation/tracing.hpp"        // for startTrace, Trace

using drogon::HttpRequestPtr;
using drogon::HttpResponse;
using drogon::HttpResponsePtr;
using drogon::HttpStatusCode;

namespace proteus::http {

void start(int port) {
  auto &app = drogon::app();

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
  std::function<void(const HttpResponsePtr &)> &&callback) {
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
  std::function<void(const HttpResponsePtr &)> &&callback) {
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
  std::string const &model) {
  PROTEUS_LOG_INFO(logger_, "Received getModelReady request");
#ifdef PROTEUS_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::kRestGet);
#endif
  (void)req;  // suppress unused variable warning

  auto resp = HttpResponse::newHttpResponse();
  try {
    if (!Manager::getInstance().workerReady(model)) {
      resp->setStatusCode(HttpStatusCode::k503ServiceUnavailable);
    }
  } catch (const std::invalid_argument &e) {
    resp->setStatusCode(HttpStatusCode::k400BadRequest);
    resp->setBody(e.what());
  }
  callback(resp);
}

void v2::ProteusHttpServer::getServerMetadata(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback) {
  PROTEUS_LOG_INFO(logger_, "Received getServerMetadata request");
#ifdef PROTEUS_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::kRestGet);
#endif
  (void)req;  // suppress unused variable warning

  NativeClient client;
  auto metadata = client.serverMetadata();

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
  const std::string &model) {
  PROTEUS_LOG_INFO(logger_, "Received getModelMetadata request");
#ifdef PROTEUS_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::kRestGet);
#endif
  (void)req;  // suppress unused variable warning

  Json::Value ret;
  bool error = false;
  try {
    auto metadata = Manager::getInstance().getWorkerMetadata(model);
    ret = ModelMetadataToJson(metadata);
  } catch (const std::invalid_argument &e) {
    ret["error"] = "Model " + model + " not found.";
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
  std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  (void)req;  // suppress unused variable warning
  NativeClient client;
  const auto models = client.modelList();

  Json::Value json;
  json["models"] = Json::arrayValue;
  for (const auto &model : models) {
    json["models"].append(model);
  }

  auto resp = HttpResponse::newHttpJsonResponse(json);
  callback(resp);
}

void v2::ProteusHttpServer::getHardware(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback) {
  PROTEUS_LOG_INFO(logger_, "Received getHardware request");
#ifdef PROTEUS_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::kRestGet);
#endif
  (void)req;  // suppress unused variable warning

  auto hw = proteus::getHardware();

  auto resp = HttpResponse::newHttpResponse();
  resp->setBody(hw);
  callback(resp);
}

void v2::ProteusHttpServer::inferModel(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback,
  std::string const &model) {
#ifdef PROTEUS_ENABLE_TRACING
  auto trace = startTrace(&(__func__[0]), req->getHeaders());
  trace->setAttribute("model", model);
#endif

  PROTEUS_LOG_INFO(logger_, "Received inferModel request for " + model);
#ifdef PROTEUS_ENABLE_METRICS
  auto now = std::chrono::high_resolution_clock::now();
  Metrics::getInstance().incrementCounter(MetricCounterIDs::kRestPost);
#endif

#ifdef PROTEUS_ENABLE_TRACING
  trace->startSpan("request_handler");
#endif

  WorkerInfo *worker = nullptr;
  try {
    worker = Manager::getInstance().getWorker(model);
  } catch (const std::invalid_argument &e) {
    PROTEUS_LOG_INFO(logger_, e.what());
    auto resp = errorHttpResponse("Worker " + model + " not found",
                                  HttpStatusCode::k400BadRequest);
#ifdef PROTEUS_ENABLE_TRACING
    auto context = trace->propagate();
    propagate(resp.get(), context);
#endif
    callback(resp);
    return;
  }

  std::unique_ptr<DrogonHttp> request;
  try {
    request = std::make_unique<DrogonHttp>(req, std::move(callback));
  } catch (const std::invalid_argument &e) {
    PROTEUS_LOG_INFO(logger_, e.what());
    auto resp = errorHttpResponse(e.what(), HttpStatusCode::k400BadRequest);
#ifdef PROTEUS_ENABLE_TRACING
    auto context = trace->propagate();
    propagate(resp.get(), context);
#endif
    callback(resp);
  }
#ifdef PROTEUS_ENABLE_METRICS
  request->set_time(now);
#endif
  auto *batcher = worker->getBatcher();
#ifdef PROTEUS_ENABLE_TRACING
  trace->endSpan();
  request->setTrace(std::move(trace));
#endif
  batcher->enqueue(std::move(request));
}

void v2::ProteusHttpServer::load(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback,
  const std::string &model) {
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

  auto hyphen_pos = model.find('-');
  std::string name;
  // if there's a hyphen in the name, currently assuming it's for xmodel. So,
  // extract the first part as the worker and the second part as the xmodel file
  // name. Put that information into the parameters with the default path for
  // KServe (/mnt/models)
  if (hyphen_pos != std::string::npos) {
    name = model.substr(0, hyphen_pos);
    auto xmodel = model.substr(hyphen_pos + 1, model.length() - hyphen_pos);
    parameters->put("model", "/mnt/models/" + model + "/" + xmodel + ".xmodel");
  } else {
    name = model;
  }

#ifdef PROTEUS_ENABLE_TRACING
  trace->setAttribute("model", name);
#endif
  PROTEUS_LOG_INFO(logger_, "Received load request is for " + name);

#ifdef PROTEUS_ENABLE_TRACING
  trace->setAttributes(parameters.get());
#endif
  std::string endpoint;
  try {
    endpoint = Manager::getInstance().loadWorker(name, *parameters);
  } catch (const std::exception &e) {
    PROTEUS_LOG_ERROR(logger_, e.what());
    auto resp = errorHttpResponse("Error loading worker " + name,
                                  HttpStatusCode::k400BadRequest);
#ifdef PROTEUS_ENABLE_TRACING
    auto context = trace->propagate();
    propagate(resp.get(), context);
#endif
    callback(resp);
  }

  auto resp = HttpResponse::newHttpResponse();
  resp->setBody(endpoint);
#ifdef PROTEUS_ENABLE_TRACING
  auto context = trace->propagate();
  propagate(resp.get(), context);
#endif
  callback(resp);
}

void v2::ProteusHttpServer::unload(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback,
  const std::string &model) {
  PROTEUS_LOG_INFO(logger_, "Received unload request");
#ifdef PROTEUS_ENABLE_TRACING
  auto trace = startTrace(&(__func__[0]), req->getHeaders());
#endif

  //   auto json = req->getJsonObject();
  //   std::string name;
  //   if (json->isMember("model_name")) {
  //     name = json->get("model_name", "").asString();
  //   } else {
  //     auto resp = errorHttpResponse("No model name specifed in unload
  //     request",
  //                                   HttpStatusCode::k400BadRequest);
  // #ifdef PROTEUS_ENABLE_TRACING
  //     auto context = trace->propagate();
  //     propagate(resp.get(), context);
  // #endif
  //     callback(resp);
  //     return;
  //   }
  std::string name = model;

#ifdef PROTEUS_ENABLE_TRACING
  trace->setAttribute("model", name);
#endif

  Manager::getInstance().unloadWorker(name);

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
  std::function<void(const HttpResponsePtr &)> &&callback) {
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
