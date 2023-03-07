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

#include "amdinfer/build_options.hpp"          // for AMDINFER_ENABLE_TRACING
#include "amdinfer/clients/http_internal.hpp"  // for propagate, errorHtt...
#include "amdinfer/core/exceptions.hpp"        // for runtime_error, inva...
#include "amdinfer/core/parameters.hpp"        // for ParameterMapPtr
#include "amdinfer/core/predict_api_internal.hpp"  // for ParameterMapPtr
#include "amdinfer/core/protocol_wrapper.hpp"      // for ProtocolWrapper
#include "amdinfer/core/shared_state.hpp"          // for SharedState
#include "amdinfer/observation/logging.hpp"       // for Logger, AMDINFER_LOG...
#include "amdinfer/observation/metrics.hpp"       // for Metrics, MetricCoun...
#include "amdinfer/observation/tracing.hpp"       // for startTrace, Trace
#include "amdinfer/servers/websocket_server.hpp"  // for WebsocketServer
#include "amdinfer/util/compression.hpp"          // for zDecompress
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

template <>
class InferenceRequestInputBuilder<std::shared_ptr<Json::Value>> {
 public:
  static InferenceRequestInput build(std::shared_ptr<Json::Value> const &req,
                                     Buffer *input_buffer, size_t offset) {
    InferenceRequestInput input;
#ifdef AMDINFER_ENABLE_LOGGING
    Logger logger{Loggers::Server};
#endif
    input.data_ = input_buffer->data(0);

    if (!req->isMember("name")) {
      throw invalid_argument("No 'name' key present in request input");
    }
    input.name_ = req->get("name", "").asString();
    if (!req->isMember("shape")) {
      throw invalid_argument("No 'shape' key present in request input");
    }
    auto shape = req->get("shape", Json::arrayValue);
    for (auto const &i : shape) {
      if (!i.isUInt64()) {
        throw invalid_argument("'shape' must be specified by uint64 elements");
      }
      input.shape_.push_back(i.asUInt64());
    }
    if (!req->isMember("datatype")) {
      throw invalid_argument("No 'datatype' key present in request input");
    }
    std::string data_type_str = req->get("datatype", "").asString();
    input.data_type_ = DataType(data_type_str.c_str());
    if (req->isMember("parameters")) {
      auto parameters = req->get("parameters", Json::objectValue);
      input.parameters_ = mapJsonToParameters(parameters);
    } else {
      input.parameters_ = std::make_unique<ParameterMap>();
    }
    if (!req->isMember("data")) {
      throw invalid_argument("No 'data' key present in request input");
    }
    auto data = req->get("data", Json::arrayValue);
    try {
      for (auto const &i : data) {
        offset = switchOverTypes(WriteData(), input.data_type_, input_buffer, i,
                                 offset);
      }
    } catch (const Json::LogicError &) {
      throw invalid_argument(
        "Could not convert some data to the provided data type");
    }
    return input;
  }
};

using InputBuilder = InferenceRequestInputBuilder<std::shared_ptr<Json::Value>>;

template <>
class InferenceRequestOutputBuilder<std::shared_ptr<Json::Value>> {
 public:
  static InferenceRequestOutput build(const std::shared_ptr<Json::Value> &req) {
    InferenceRequestOutput output;
    output.data_ = nullptr;
    output.name_ = req->get("name", "").asString();
    if (req->isMember("parameters")) {
      auto parameters = req->get("parameters", Json::objectValue);
      output.parameters_ = mapJsonToParameters(parameters);
    } else {
      output.parameters_ = std::make_unique<ParameterMap>();
    }
    return output;
  }
};

using OutputBuilder =
  InferenceRequestOutputBuilder<std::shared_ptr<Json::Value>>;

InferenceRequestPtr RequestBuilder::build(
  const std::shared_ptr<Json::Value> &req, const BufferRawPtrs &input_buffers,
  std::vector<size_t> &input_offsets, const BufferRawPtrs &output_buffers,
  std::vector<size_t> &output_offsets) {
  auto request = std::make_shared<InferenceRequest>();

  if (req->isMember("id")) {
    request->id_ = req->get("id", "").asString();
  } else {
    request->id_ = "";
  }
  if (req->isMember("parameters")) {
    auto parameters = req->get("parameters", Json::objectValue);
    request->parameters_ = mapJsonToParameters(parameters);
  } else {
    request->parameters_ = std::make_unique<ParameterMap>();
  }

  if (!req->isMember("inputs")) {
    throw invalid_argument("No 'inputs' key present in request");
  }
  auto inputs = req->get("inputs", Json::arrayValue);
  if (!inputs.isArray()) {
    throw invalid_argument("'inputs' is not an array");
  }

  request->callback_ = nullptr;

  assert(input_buffers.size() == inputs.size());
  const auto input_num = input_buffers.size();
  for (auto i = 0U; i < input_num; ++i) {
    const auto &json_input = inputs[i];
    if (!json_input.isObject()) {
      throw invalid_argument("At least one element in 'inputs' is not an obj");
    }
    auto *buffer = input_buffers[i];
    auto &offset = input_offsets[i];
    auto input = InputBuilder::build(std::make_shared<Json::Value>(json_input),
                                     buffer, offset);
    offset += (input.getSize() * input.getDatatype().size());

    request->inputs_.push_back(std::move(input));
  }

  // TODO(varunsh): output_offset is currently ignored! The size of the output
  // needs to come from the worker but we have no such information.
  const auto output_num = output_buffers.size();
  if (req->isMember("outputs")) {
    auto outputs = req->get("outputs", Json::arrayValue);
    assert(output_buffers.size() == outputs.size());
    for (auto i = 0U; i < output_num; ++i) {
      const auto &json_output = outputs[i];
      auto *buffer = output_buffers[i];
      auto &offset = output_offsets[i];

      auto output =
        OutputBuilder::build(std::make_shared<Json::Value>(json_output));
      output.setData(static_cast<std::byte *>(buffer->data(offset)));
      request->outputs_.push_back(std::move(output));
      // output += request->outputs_.back().getSize(); // see TODO
    }
  } else {
    for (auto i = 0U; i < output_num; ++i) {
      auto *buffer = output_buffers[i];
      const auto &offset = output_offsets[i];

      request->outputs_.emplace_back();
      request->outputs_.back().setData(
        static_cast<std::byte *>(buffer->data(offset)));
    }
  }

  return request;
}

using RequestBuilder = InferenceRequestBuilder<std::shared_ptr<Json::Value>>;

drogon::HttpResponsePtr errorHttpResponse(const std::string &error,
                                          int status_code) {
  Json::Value ret;
  ret["error"] = error.data();
  auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
  resp->setStatusCode(static_cast<drogon::HttpStatusCode>(status_code));
  return resp;
}

using DrogonCallback = std::function<void(const drogon::HttpResponsePtr &)>;

/**
 * @brief The DrogonHttp ProtocolWrapper class encapsulates incoming requests
 * from Drogon's HTTP interface to the batcher.
 *
 */
class DrogonHttp : public ProtocolWrapper {
 public:
  /**
   * @brief Construct a new DrogonHttp object
   *
   * @param req
   * @param callback
   */
  DrogonHttp(const drogon::HttpRequestPtr &req, DrogonCallback callback)
    : req_(req), callback_(std::move(callback)) {
    this->json_ = parseJson(req_.get());
  }

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
      Callback callback = [callback = std::move(this->callback_)](
                            const InferenceResponse &response) {
        drogon::HttpResponsePtr resp;
        if (response.isError()) {
          resp = errorHttpResponse(response.getError(),
                                   HttpStatusCode::k400BadRequest);
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
      return request;
    } catch (const invalid_argument &e) {
      AMDINFER_LOG_INFO(logger, e.what());
      this->callback_(
        errorHttpResponse(e.what(), HttpStatusCode::k400BadRequest));
      return nullptr;
    }
  };

  size_t getInputSize() override {
    if (!this->json_->isMember("inputs")) {
      throw invalid_argument("No 'inputs' key present in request");
    }
    auto inputs = this->json_->get("inputs", Json::arrayValue);
    if (!inputs.isArray()) {
      throw invalid_argument("'inputs' is not an array");
    }

    return inputs.size();
  };

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
  };
  void errorHandler(const std::exception &e) override {
#ifdef AMDINFER_ENABLE_LOGGING
    [[maybe_unused]] const auto &logger = this->getLogger();
    AMDINFER_LOG_DEBUG(logger, e.what());
#endif
    this->callback_(
      errorHttpResponse(e.what(), HttpStatusCode::k400BadRequest));
  };

 private:
  drogon::HttpRequestPtr req_;
  DrogonCallback callback_;
  std::shared_ptr<Json::Value> json_;
};

HttpServer::HttpServer(SharedState *state) : state_(state) {
  AMDINFER_LOG_DEBUG(logger_, "Constructed HttpServer");
}

#ifdef AMDINFER_ENABLE_REST

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

  try {
    auto request = std::make_unique<DrogonHttp>(req, std::move(callback));
#ifdef AMDINFER_ENABLE_METRICS
    request->setTime(now);
#endif
#ifdef AMDINFER_ENABLE_TRACING
    trace->endSpan();
    request->setTrace(std::move(trace));
#endif
    state_->modelInfer(model, std::move(request));
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
  ParameterMapPtr parameters = nullptr;
  if (json != nullptr) {
    parameters = mapJsonToParameters(*json);
  } else {
    parameters = std::make_unique<ParameterMap>();
  }
#ifdef AMDINFER_ENABLE_TRACING
  trace->setAttributes(parameters.get());
#endif

  try {
    state_->modelLoad(model_lower, parameters.get());
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
  ParameterMapPtr parameters = nullptr;
  if (json != nullptr) {
    parameters = mapJsonToParameters(*json);
  } else {
    parameters = std::make_unique<ParameterMap>();
  }

  auto worker_lower = util::toLower(worker);

#ifdef AMDINFER_ENABLE_TRACING
  trace->setAttribute("model", worker_lower);
#endif
  AMDINFER_LOG_INFO(logger_, "Received load request is for " + worker_lower);

#ifdef AMDINFER_ENABLE_TRACING
  trace->setAttributes(parameters.get());
#endif
  HttpResponsePtr resp;
  try {
    auto endpoint = state_->workerLoad(worker_lower, parameters.get());
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

#endif  // AMDINFER_ENABLE_REST

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
