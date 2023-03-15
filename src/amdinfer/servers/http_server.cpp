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

/**
 * @file
 * @brief Implements the HTTP REST server
 */

#include "amdinfer/servers/http_server.hpp"

#include <drogon/HttpAppFramework.h>  // for HttpAppFramework, app
#include <drogon/HttpRequest.h>       // for HttpRequestPtr, Htt...
#include <json/value.h>               // for Value, arrayValue
#include <trantor/utils/Logger.h>     // for Logger, Logger::Warn

#include <chrono>         // for high_resolution_clock
#include <memory>         // for shared_ptr, __share...
#include <string>         // for allocator, operator+
#include <unordered_set>  // for unordered_set
#include <utility>        // for move
#include <vector>         // for vector

#include "amdinfer/buffers/buffer.hpp"         // for BufferPtr
#include "amdinfer/build_options.hpp"          // for AMDINFER_ENABLE_TRACING
#include "amdinfer/clients/http_internal.hpp"  // for propagate, errorHtt...
#include "amdinfer/core/exceptions.hpp"        // for runtime_error, inva...
#include "amdinfer/core/parameters.hpp"        // for ParameterMap
#include "amdinfer/core/predict_api_internal.hpp"  // for ParameterMap
#include "amdinfer/core/shared_state.hpp"          // for SharedState
#include "amdinfer/observation/logging.hpp"       // for Logger, AMDINFER_LOG...
#include "amdinfer/observation/metrics.hpp"       // for Metrics, MetricCoun...
#include "amdinfer/observation/tracing.hpp"       // for startTrace, Trace
#include "amdinfer/servers/websocket_server.hpp"  // for WebsocketServer
#include "amdinfer/util/compression.hpp"          // for zDecompress
#include "amdinfer/util/containers.hpp"           // for containerProduct
#include "amdinfer/util/string.hpp"               // for toLower

using drogon::HttpRequestPtr;
using drogon::HttpResponse;
using drogon::HttpResponsePtr;
using drogon::HttpStatusCode;

namespace amdinfer {

namespace http {

void start(SharedState *state, uint16_t port) {
  auto controller = std::make_shared<HttpServer>(state);
  auto ws_controller = std::make_shared<WebsocketServer>(state);

  auto &app = drogon::app();
  app.registerController(controller);
  app.registerController(ws_controller);
#ifdef AMDINFER_ENABLE_LOGGING
  auto dir = getLogDirectory();
  app.setLogLevel(trantor::Logger::kWarn).setLogPath(dir);
#else
  app.setLogLevel(trantor::Logger::kFatal).setLogPath(".");
#endif

  app.addListener("0.0.0.0", port)
    .setThreadNum(kDefaultDrogonThreads)
    .registerPostHandlingAdvice([](const drogon::HttpRequestPtr &req,
                                   const drogon::HttpResponsePtr &resp) {
      (void)req;  // suppress unused variable warning
      resp->addHeader("Access-Control-Allow-Origin", "*");
    })
    .setClientMaxBodySize(kMaxClientBodySize)
    .disableSigtermHandling()
    // .enableRunAsDaemon()
    .run();
}

void stop() { drogon::app().quit(); }

}  // namespace http

std::shared_ptr<Json::Value> parseJson(const drogon::HttpRequest *req) {
#ifdef AMDINFER_ENABLE_LOGGING
  Logger logger{Loggers::Server};
#endif

  // attempt to get the JSON object directly first
  const auto &json_obj = req->getJsonObject();
  if (json_obj != nullptr) {
    return json_obj;
  }

  AMDINFER_LOG_DEBUG(logger, "Failed to get JSON data directly");

  // if it's not valid, then we need to attempt to parse the body
  auto root = std::make_shared<Json::Value>();

  std::string errors;
  Json::CharReaderBuilder builder;
  Json::CharReader *reader = builder.newCharReader();
  auto body = req->getBody();
  bool success =
    reader->parse(body.data(), body.data() + body.size(), root.get(), &errors);
  if (success) {
    return root;
  }

  AMDINFER_LOG_DEBUG(logger, "Failed to interpret body as JSON data");

  // if it's still not valid, attempt to uncompress the body and convert to JSON
  auto body_decompress = util::zDecompress(body.data(), body.length());
  success = reader->parse(body_decompress.data(),
                          body_decompress.data() + body_decompress.size(),
                          root.get(), &errors);
  if (success) {
    return root;
  }

  throw invalid_argument("Failed to interpret request body as JSON");
}

Json::Value parseResponse(InferenceResponse response) {
  Json::Value ret;
  ret["model_name"] = response.getModel();
  ret["outputs"] = Json::arrayValue;
  ret["id"] = response.getID();
  auto outputs = response.getOutputs();
  for (const InferenceResponseOutput &output : outputs) {
    Json::Value json_output;
    json_output["name"] = output.getName();
    json_output["parameters"] = Json::objectValue;
    json_output["data"] = Json::arrayValue;
    json_output["shape"] = Json::arrayValue;
    json_output["datatype"] = output.getDatatype().str();
    const auto &shape = output.getShape();
    for (const size_t &index : shape) {
      json_output["shape"].append(static_cast<Json::UInt>(index));
    }

    switchOverTypes(SetInputData(), output.getDatatype(),
                    &(json_output["data"]), output.getData(), output.getSize());
    ret["outputs"].append(json_output);
  }
  return ret;
}

struct WriteData {
  template <typename T>
  size_t operator()(Buffer *buffer, const Json::Value &value,
                    size_t offset) const {
    if constexpr (std::is_same_v<T, char>) {
      return buffer->write(jsonValueToType<T>(value), offset);
    } else {
      return buffer->write(static_cast<T>(jsonValueToType<T>(value)), offset);
    }
  }
};

drogon::HttpResponsePtr errorHttpResponse(const std::string &error,
                                          int status_code) {
  Json::Value ret;
  ret["error"] = error.data();
  auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
  resp->setStatusCode(static_cast<drogon::HttpStatusCode>(status_code));
  return resp;
}

using DrogonCallback = std::function<void(const drogon::HttpResponsePtr &)>;

HttpServer::HttpServer(SharedState *state) : state_(state) {
  AMDINFER_LOG_DEBUG(logger_, "Constructed HttpServer");
}

#ifdef AMDINFER_ENABLE_HTTP

void HttpServer::getServerLive(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback) const {
  AMDINFER_LOG_INFO(logger_, "Received getServerLive request");
#ifdef AMDINFER_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::RestGet);
#endif
#ifdef AMDINFER_ENABLE_TRACING
  auto trace = startTrace(&(__func__[0]), req->getHeaders());
#else
  (void)req;  // suppress unused variable warning
#endif

  auto resp = HttpResponse::newHttpResponse();
#ifdef AMDINFER_ENABLE_TRACING
  auto context = trace->propagate();
  propagate(resp.get(), context);
#endif
  callback(resp);
}

void HttpServer::getServerReady(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback) const {
  AMDINFER_LOG_INFO(logger_, "Received getServerReady request");
#ifdef AMDINFER_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::RestGet);
#endif
  (void)req;  // suppress unused variable warning

  // for now, assuming that server is always ready (assumes that user has loaded
  // all the required models).

  auto resp = HttpResponse::newHttpResponse();
  callback(resp);
}

void HttpServer::getModelReady(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback,
  std::string const &model) const {
  AMDINFER_LOG_INFO(logger_, "Received getModelReady request");
#ifdef AMDINFER_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::RestGet);
#endif
  (void)req;  // suppress unused variable warning

  auto resp = HttpResponse::newHttpResponse();
  try {
    if (!state_->modelReady(model)) {
      resp->setStatusCode(HttpStatusCode::k503ServiceUnavailable);
    }
  } catch (const invalid_argument &e) {
    resp->setStatusCode(HttpStatusCode::k400BadRequest);
    resp->setBody(e.what());
  }
  callback(resp);
}

void HttpServer::getServerMetadata(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback) const {
  AMDINFER_LOG_INFO(logger_, "Received getServerMetadata request");
#ifdef AMDINFER_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::RestGet);
#endif
  (void)req;  // suppress unused variable warning

  auto metadata = SharedState::serverMetadata();

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

void HttpServer::getModelMetadata(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback,
  const std::string &model) const {
  AMDINFER_LOG_INFO(logger_, "Received getModelMetadata request");
#ifdef AMDINFER_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::RestGet);
#endif
  (void)req;  // suppress unused variable warning

  Json::Value ret;
  bool error = false;
  try {
    auto metadata = state_->modelMetadata(model);
    ret = modelMetadataToJson(metadata);
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

void HttpServer::modelList(
  const drogon::HttpRequestPtr &req,
  std::function<void(const drogon::HttpResponsePtr &)> &&callback) const {
  AMDINFER_LOG_INFO(logger_, "Received modelList request");
  (void)req;  // suppress unused variable warning
  const auto models = state_->modelList();

  Json::Value json;
  json["models"] = Json::arrayValue;
  for (const auto &model : models) {
    json["models"].append(model);
  }

  auto resp = HttpResponse::newHttpJsonResponse(json);
  callback(resp);
}

void HttpServer::hasHardware(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback) const {
  AMDINFER_LOG_INFO(logger_, "Received hasHardware request");
#ifdef AMDINFER_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::RestGet);
#endif
  const auto &json = req->jsonObject();

  if (json == nullptr) {
    auto resp = errorHttpResponse("No JSON body in hasHardware request",
                                  HttpStatusCode::k400BadRequest);
    callback(resp);
    return;
  }

  auto found = SharedState::hasHardware(json->get("name", "").asString(),
                                        json->get("num", 1).asInt());

  auto resp = HttpResponse::newHttpResponse();
  if (!found) {
    resp->setStatusCode(drogon::k404NotFound);
  }
  callback(resp);
}

InferenceRequestInput getInput(const Json::Value &json,
                               const MemoryPool *pool) {
  InferenceRequestInput input;

  input.setData(nullptr);

  if (!json.isMember("name")) {
    throw invalid_argument("No 'name' key present in request input");
  }
  input.setName(json.get("name", "").asString());
  if (!json.isMember("shape")) {
    throw invalid_argument("No 'shape' key present in request input");
  }
  auto shape = json.get("shape", Json::arrayValue);
  std::vector<uint64_t> shape_vector;
  shape_vector.reserve(shape.size());
  for (auto const &i : shape) {
    if (!i.isUInt64()) {
      throw invalid_argument("'shape' must be specified by uint64 elements");
    }
    shape_vector.push_back(i.asUInt64());
  }
  input.setShape(shape_vector);

  if (!json.isMember("datatype")) {
    throw invalid_argument("No 'datatype' key present in request input");
  }
  // getting it as CString didn't work elsewhere
  auto data_type_str = json.get("datatype", "").asString();
  input.setDatatype(DataType(data_type_str.c_str()));
  if (json.isMember("parameters")) {
    auto parameters = json.get("parameters", Json::objectValue);
    input.setParameters(mapJsonToParameters(parameters));
  }

  auto buffer = pool->get({MemoryAllocators::Cpu}, input, 1);
  if (!json.isMember("data")) {
    throw invalid_argument("No 'data' key present in request input");
  }
  auto data = json.get("data", Json::arrayValue);
  size_t offset = 0;
  input.setData(buffer->data(offset));
  try {
    for (auto const &i : data) {
      offset = switchOverTypes(WriteData(), input.getDatatype(), buffer.get(),
                               i, offset);
    }
  } catch (const Json::LogicError &) {
    throw invalid_argument(
      "Could not convert some data to the provided data type");
  }
  return input;
}

InferenceRequestOutput getOutput(const Json::Value &json) {
  InferenceRequestOutput output;
  output.setData(nullptr);
  output.setName(json.get("name", "").asString());
  if (json.isMember("parameters")) {
    auto parameters = json.get("parameters", Json::objectValue);
    output.setParameters(mapJsonToParameters(parameters));
  }
  return output;
}

void setCallback(InferenceRequest *request, DrogonCallback &&drogon_callback) {
  Callback callback = [callback = std::move(drogon_callback)](
                        const InferenceResponse &response) {
    drogon::HttpResponsePtr resp;
    if (response.isError()) {
      resp =
        errorHttpResponse(response.getError(), HttpStatusCode::k400BadRequest);
    } else {
      try {
        Json::Value ret = parseResponse(response);
        resp = drogon::HttpResponse::newHttpJsonResponse(ret);
      } catch (const invalid_argument &e) {
        resp = errorHttpResponse(e.what(), HttpStatusCode::k400BadRequest);
      }
    }
#ifdef AMDINFER_ENABLE_TRACING
    const auto &context = response.getContext();
    propagate(resp.get(), context);
#endif
    callback(resp);
  };
  request->setCallback(std::move(callback));
}

InferenceRequestPtr getRequest(const std::shared_ptr<Json::Value> &json,
                               const MemoryPool *pool) {
  auto request = std::make_shared<InferenceRequest>();

  if (json->isMember("id")) {
    request->setID(json->get("id", "").asString());
  } else {
    request->setID("");
  }
  if (json->isMember("parameters")) {
    auto parameters = json->get("parameters", Json::objectValue);
    request->setParameters(mapJsonToParameters(parameters));
  }

  if (!json->isMember("inputs")) {
    throw invalid_argument("No 'inputs' key present in request");
  }
  auto inputs = json->get("inputs", Json::arrayValue);
  if (!inputs.isArray()) {
    throw invalid_argument("'inputs' is not an array");
  }

  request->setCallback(nullptr);

  const auto input_num = inputs.size();
  for (auto i = 0U; i < input_num; ++i) {
    const auto &input = inputs[i];
    if (!input.isObject()) {
      throw invalid_argument("At least one element in 'inputs' is not an obj");
    }
    request->addInputTensor(getInput(input, pool));
  }

  if (json->isMember("outputs")) {
    auto outputs = json->get("outputs", Json::arrayValue);
    const auto output_num = outputs.size();
    for (auto i = 0U; i < output_num; ++i) {
      const auto &json_output = outputs[i];

      request->addOutputTensor(getOutput(json_output));
    }
  }

  return request;
}

void HttpServer::modelInfer(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback,
  std::string const &model) const {
#ifdef AMDINFER_ENABLE_TRACING
  auto trace = startTrace(&(__func__[0]), req->getHeaders());
  trace->setAttribute("model", model);
#endif

  AMDINFER_LOG_INFO(logger_, "Received modelInfer request for " + model);
#ifdef AMDINFER_ENABLE_METRICS
  auto now = std::chrono::high_resolution_clock::now();
  Metrics::getInstance().incrementCounter(MetricCounterIDs::RestPost);
#endif

#ifdef AMDINFER_ENABLE_TRACING
  trace->startSpan("request_handler");
#endif

  auto json = req->getJsonObject();
  try {
    auto request = getRequest(json, state_->getPool());
    setCallback(request.get(), std::move(callback));
    auto request_container = std::make_unique<RequestContainer>();
    request_container->request = request;
#ifdef AMDINFER_ENABLE_METRICS
    request_container->start_time = now;
#endif
#ifdef AMDINFER_ENABLE_TRACING
    trace->endSpan();
    request_container->trace = std::move(trace);
#endif
    state_->modelInfer(model, std::move(request_container));
  } catch (const invalid_argument &e) {
    AMDINFER_LOG_INFO(logger_, e.what());
    auto resp = errorHttpResponse(e.what(), HttpStatusCode::k400BadRequest);
#ifdef AMDINFER_ENABLE_TRACING
    auto context = trace->propagate();
    propagate(resp.get(), context);
#endif
    callback(resp);
  }
}

void HttpServer::modelLoad(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback,
  const std::string &model) const {
  auto model_lower = util::toLower(model);
#ifdef AMDINFER_ENABLE_TRACING
  auto trace = startTrace(&(__func__[0]), req->getHeaders());
  trace->setAttribute("model", model_lower);
#endif
  AMDINFER_LOG_INFO(logger_, "Received modelLoad request for " + model_lower);

  auto json = req->getJsonObject();
  ParameterMap parameters;
  if (json != nullptr) {
    parameters = mapJsonToParameters(*json);
  }
#ifdef AMDINFER_ENABLE_TRACING
  trace->setAttributes(parameters);
#endif

  try {
    state_->modelLoad(model_lower, parameters);
  } catch (const runtime_error &e) {
    AMDINFER_LOG_ERROR(logger_, e.what());
    auto resp = errorHttpResponse(e.what(), HttpStatusCode::k400BadRequest);
#ifdef AMDINFER_ENABLE_TRACING
    auto context = trace->propagate();
    propagate(resp.get(), context);
#endif
    callback(resp);
  }

  auto resp = HttpResponse::newHttpResponse();
#ifdef AMDINFER_ENABLE_TRACING
  auto context = trace->propagate();
  propagate(resp.get(), context);
#endif
  callback(resp);
}

void HttpServer::modelUnload(
  [[maybe_unused]] const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback,
  const std::string &model) const {
  AMDINFER_LOG_INFO(logger_, "Received modelUnload request");
#ifdef AMDINFER_ENABLE_TRACING
  auto trace = startTrace(&(__func__[0]), req->getHeaders());
#endif

  auto model_lower = util::toLower(model);

#ifdef AMDINFER_ENABLE_TRACING
  trace->setAttribute("model", model_lower);
#endif

  state_->modelUnload(model_lower);

  auto resp = HttpResponse::newHttpResponse();
#ifdef AMDINFER_ENABLE_TRACING
  auto context = trace->propagate();
  propagate(resp.get(), context);
#endif
  callback(resp);
}

void HttpServer::workerLoad(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback,
  const std::string &worker) const {
  AMDINFER_LOG_INFO(logger_, "Received load request");
#ifdef AMDINFER_ENABLE_TRACING
  auto trace = startTrace(&(__func__[0]), req->getHeaders());
#endif

  auto json = req->getJsonObject();
  ParameterMap parameters;
  if (json != nullptr) {
    parameters = mapJsonToParameters(*json);
  }

  auto worker_lower = util::toLower(worker);

#ifdef AMDINFER_ENABLE_TRACING
  trace->setAttribute("model", worker_lower);
#endif
  AMDINFER_LOG_INFO(logger_, "Received load request is for " + worker_lower);

#ifdef AMDINFER_ENABLE_TRACING
  trace->setAttributes(parameters);
#endif
  HttpResponsePtr resp;
  try {
    auto endpoint = state_->workerLoad(worker_lower, parameters);
    resp = HttpResponse::newHttpResponse();
    resp->setBody(endpoint);
  } catch (const runtime_error &e) {
    AMDINFER_LOG_ERROR(logger_, e.what());
    resp = errorHttpResponse(e.what(), HttpStatusCode::k400BadRequest);
  }

#ifdef AMDINFER_ENABLE_TRACING
  auto context = trace->propagate();
  propagate(resp.get(), context);
#endif
  callback(resp);
}

void HttpServer::workerUnload(
  [[maybe_unused]] const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback,
  const std::string &worker) const {
#ifdef AMDINFER_ENABLE_TRACING
  auto trace = startTrace(&(__func__[0]), req->getHeaders());
#endif

  auto worker_lower = util::toLower(worker);

  AMDINFER_LOG_INFO(logger_, "Received unload request is for " + worker_lower);

#ifdef AMDINFER_ENABLE_TRACING
  trace->setAttribute("model", worker_lower);
#endif

  state_->workerUnload(worker_lower);

  auto resp = HttpResponse::newHttpResponse();
#ifdef AMDINFER_ENABLE_TRACING
  auto context = trace->propagate();
  propagate(resp.get(), context);
#endif
  callback(resp);
}

#endif  // AMDINFER_ENABLE_HTTP

#ifdef AMDINFER_ENABLE_METRICS
void HttpServer::metrics(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback) const {
  (void)req;  // suppress unused variable warning
  AMDINFER_LOG_INFO(logger_, "Received metrics request");
  std::string body = Metrics::getInstance().getMetrics();
  auto resp = drogon::HttpResponse::newHttpResponse();
  resp->setBody(body);
  resp->setContentTypeCode(drogon::ContentType::CT_TEXT_PLAIN);
  callback(resp);
}
#endif

}  // namespace amdinfer
