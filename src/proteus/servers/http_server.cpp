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
#include <drogon/HttpRequest.h>       // for HttpRequest
#include <json/config.h>              // for Int64, UInt, UInt64
#include <json/reader.h>              // for CharReaderBuilder, CharR...
#include <json/value.h>               // for Value, arrayValue, objec...
#include <trantor/utils/Logger.h>     // for Logger, Logger::kWarn

#include <chrono>       // for high_resolution_clock
#include <cstdint>      // for int16_t, int32_t, int64_t
#include <exception>    // for exception
#include <iostream>     // for operator<<, cout, ostream
#include <memory>       // for allocator, shared_ptr
#include <string>       // for operator+, string, basic...
#include <string_view>  // for basic_string_view
#include <utility>      // for move
#include <vector>       // for vector, _Bit_reference

#include "proteus/batching/batcher.hpp"      // for Batcher
#include "proteus/build_options.hpp"         // for PROTEUS_ENABLE_TRACING
#include "proteus/clients/native.hpp"        // for getHardware
#include "proteus/core/data_types.hpp"       // for DataType, mapTypeToStr
#include "proteus/core/manager.hpp"          // for Manager
#include "proteus/core/predict_api.hpp"      // for RequestParametersPtr
#include "proteus/core/worker_info.hpp"      // for WorkerInfo
#include "proteus/helpers/compression.hpp"   // for z_decompress
#include "proteus/helpers/declarations.hpp"  // for InferenceResponseOutput
#include "proteus/observation/logging.hpp"   // for SPDLOG_LOGGER_INFO, getL...
#include "proteus/observation/metrics.hpp"   // for Metrics, MetricIDs, Metr...
#include "proteus/observation/tracing.hpp"   // for startSpan, Span, setTags
#include "proteus/version.hpp"               // for kProteusVersion

using drogon::HttpRequestPtr;
using drogon::HttpResponse;
using drogon::HttpResponsePtr;
using drogon::HttpStatusCode;

namespace proteus::http {

drogon::HttpResponsePtr errorHttpResponse(std::string_view error,
                                          int status_code) {
  Json::Value ret;
  ret["error"] = error.data();
  auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
  resp->setStatusCode(static_cast<drogon::HttpStatusCode>(status_code));
  return resp;
}

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

#ifdef PROTEUS_ENABLE_TRACING
void propagate(HttpResponse *resp, const StringMap &context) {
  for (const auto &[key, value] : context) {
    resp->addHeader(key, value);
  }
}
#endif

v2::ProteusHttpServer::ProteusHttpServer() {
#ifdef PROTEUS_ENABLE_LOGGING
  this->logger_ = getLogger();
#endif
  SPDLOG_LOGGER_DEBUG(this->logger_, "Constructed v2::ProteusHttpServer");
}

#ifdef PROTEUS_ENABLE_REST

void v2::ProteusHttpServer::getServerLive(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback) {
  SPDLOG_LOGGER_INFO(this->logger_, "Received getServerLive request");
#ifdef PROTEUS_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::kRestGet);
#endif
#ifdef PROTEUS_ENABLE_TRACING
  auto trace = startTrace(__func__, req->getHeaders());
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
  SPDLOG_LOGGER_INFO(this->logger_, "Received getServerReady request");
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
  SPDLOG_LOGGER_INFO(this->logger_, "Received getModelReady request");
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
  }
  callback(resp);
}

void v2::ProteusHttpServer::getServerMetadata(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback) {
  SPDLOG_LOGGER_INFO(this->logger_, "Received getServerMetadata request");
#ifdef PROTEUS_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::kRestGet);
#endif
  (void)req;  // suppress unused variable warning

  Json::Value ret;
  ret["name"] = "proteus";
  ret["version"] = kProteusVersion;
  ret["extensions"] = Json::arrayValue;
#ifdef PROTEUS_ENABLE_AKS
  ret["extensions"].append("aks");
#endif
#ifdef PROTEUS_ENABLE_VITIS
  ret["extensions"].append("vitis");
#endif
  auto resp = HttpResponse::newHttpJsonResponse(ret);
  callback(resp);
}

void v2::ProteusHttpServer::getModelMetadata(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback,
  const std::string &model) {
  SPDLOG_LOGGER_INFO(this->logger_, "Received getModelMetadata request");
#ifdef PROTEUS_ENABLE_METRICS
  Metrics::getInstance().incrementCounter(MetricCounterIDs::kRestGet);
#endif
  (void)req;  // suppress unused variable warning

  Json::Value ret;
  bool error = false;
  try {
    auto metadata = Manager::getInstance().getWorkerMetadata(model);
    ret = metadata.toJson();
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

void v2::ProteusHttpServer::getHardware(
  const HttpRequestPtr &req,
  std::function<void(const HttpResponsePtr &)> &&callback) {
  SPDLOG_LOGGER_INFO(this->logger_, "Received getHardware request");
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
  auto trace = startTrace(__func__, req->getHeaders());
  trace->setAttribute("model", model);
#endif

  SPDLOG_LOGGER_INFO(this->logger_, "Received inferModel request for " + model);
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
    SPDLOG_LOGGER_INFO(this->logger_, e.what());
    auto resp = errorHttpResponse("Worker " + model + " not found",
                                  HttpStatusCode::k400BadRequest);
#ifdef PROTEUS_ENABLE_TRACING
    auto context = trace->propagate();
    propagate(resp.get(), context);
#endif
    callback(resp);
    return;
  }

  auto request = std::make_unique<DrogonHttp>(req, std::move(callback));
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
  std::function<void(const HttpResponsePtr &)> &&callback) {
  SPDLOG_LOGGER_INFO(this->logger_, "Received load request");
#ifdef PROTEUS_ENABLE_TRACING
  auto trace = startTrace(__func__, req->getHeaders());
#endif

  auto json = req->getJsonObject();
  std::string name;
  if (json->isMember("model_name")) {
    name = json->get("model_name", "").asString();
  } else {
    auto resp = errorHttpResponse("No model name specifed in load request",
                                  HttpStatusCode::k400BadRequest);
#ifdef PROTEUS_ENABLE_TRACING
    auto context = trace->propagate();
    propagate(resp.get(), context);
#endif
    callback(resp);
    return;
  }
#ifdef PROTEUS_ENABLE_TRACING
  trace->setAttribute("model", name);
#endif
  SPDLOG_LOGGER_INFO(this->logger_, "Received load request is for " + name);

  RequestParametersPtr parameters = nullptr;
  if (json->isMember("parameters")) {
    auto json_parameters = json->get("parameters", "");
    parameters = addParameters(json_parameters);
  } else {
    parameters = std::make_unique<RequestParameters>();
  }

#ifdef PROTEUS_ENABLE_TRACING
  trace->setAttributes(parameters.get());
#endif
  std::string endpoint;
  try {
    endpoint = Manager::getInstance().loadWorker(name, *parameters);
  } catch (const std::exception &e) {
    SPDLOG_LOGGER_ERROR(this->logger_, e.what());
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
  std::function<void(const HttpResponsePtr &)> &&callback) {
  SPDLOG_LOGGER_INFO(this->logger_, "Received unload request");
#ifdef PROTEUS_ENABLE_TRACING
  auto trace = startTrace(__func__, req->getHeaders());
#endif

  auto json = req->getJsonObject();
  std::string name;
  if (json->isMember("model_name")) {
    name = json->get("model_name", "").asString();
  } else {
    auto resp = errorHttpResponse("No model name specifed in unload request",
                                  HttpStatusCode::k400BadRequest);
#ifdef PROTEUS_ENABLE_TRACING
    auto context = trace->propagate();
    propagate(resp.get(), context);
#endif
    callback(resp);
    return;
  }

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
  SPDLOG_LOGGER_INFO(this->logger_, "Received metrics request");
  std::string body = Metrics::getInstance().getMetrics();
  auto resp = drogon::HttpResponse::newHttpResponse();
  resp->setBody(body);
  resp->setContentTypeCode(drogon::ContentType::CT_TEXT_PLAIN);
  callback(resp);
}
#endif

DrogonHttp::DrogonHttp(const drogon::HttpRequestPtr &req,
                       DrogonCallback callback) {
  this->req_ = req;
  this->callback_ = std::move(callback);
  this->type_ = InterfaceType::kDrogonHttp;
  this->json_ = nullptr;
}

void DrogonHttp::setJson() {
  const auto &json_raw = this->req_->getJsonObject();

  // if we fail to get the JSON object, return
  if (json_raw == nullptr) {
    auto root = std::make_shared<Json::Value>();
    std::string errors;
    Json::CharReaderBuilder builder;
    Json::CharReader *reader = builder.newCharReader();
    auto body = this->req_->getBody();
    auto body_parsed = z_decompress(body.data(), body.length());
    if (body_parsed.empty()) {
      this->callback_(errorHttpResponse("Failed attempt to inflate request",
                                        HttpStatusCode::k400BadRequest));
      return;
    }
    bool parsingSuccessful =
      reader->parse(body_parsed.data(), body_parsed.data() + body_parsed.size(),
                    root.get(), &errors);
    if (!parsingSuccessful) {
      this->callback_(errorHttpResponse("Failed to parse JSON request",
                                        HttpStatusCode::k400BadRequest));
      return;
    }
    SPDLOG_LOGGER_INFO(this->logger_, "Successfully inflated request");
    this->json_ = std::move(root);
  } else {
    this->json_ = json_raw;
  }
}

size_t DrogonHttp::getInputSize() {
  if (this->json_ == nullptr) {
    this->setJson();
  }

  if (!this->json_->isMember("inputs")) {
    throw std::invalid_argument("No 'inputs' key present in request");
  }
  auto inputs = this->json_->get("inputs", Json::arrayValue);
  if (!inputs.isArray()) {
    throw std::invalid_argument("'inputs' is not an array");
  }
  return inputs.size();
}

using types::DataType;

Json::Value parseResponse(InferenceResponse response) {
  Json::Value ret;
  ret["model_name"] = response.getModel();
  ret["outputs"] = Json::arrayValue;
  ret["id"] = response.getID();
  auto outputs = response.getOutputs();
  for (InferenceResponseOutput &output : outputs) {
    Json::Value json_output;
    json_output["name"] = output.getName();
    json_output["parameters"] = Json::objectValue;
    json_output["data"] = Json::arrayValue;
    json_output["shape"] = Json::arrayValue;
    json_output["datatype"] = types::mapTypeToStr(output.getDatatype());
    auto shape = output.getShape();
    for (const size_t &index : shape) {
      json_output["shape"].append(static_cast<Json::UInt>(index));
    }

    switch (output.getDatatype()) {
      case DataType::BOOL: {
        auto *data = static_cast<std::vector<bool> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append(static_cast<bool>((*data)[i]));
        }
        break;
      }
      case DataType::UINT8: {
        auto *data = static_cast<std::vector<uint8_t> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append((*data)[i]);
        }
        break;
      }
      case DataType::UINT16: {
        auto *data = static_cast<std::vector<uint16_t> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append((*data)[i]);
        }
        break;
      }
      case DataType::UINT32: {
        auto *data = static_cast<std::vector<uint32_t> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append((*data)[i]);
        }
        break;
      }
      case DataType::UINT64: {
        auto *data = static_cast<std::vector<uint64_t> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append(static_cast<Json::UInt64>((*data)[i]));
        }
        break;
      }
      case DataType::INT8: {
        auto *data = static_cast<std::vector<int8_t> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append((*data)[i]);
        }
        break;
      }
      case DataType::INT16: {
        auto *data = static_cast<std::vector<int16_t> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append((*data)[i]);
        }
        break;
      }
      case DataType::INT32: {
        auto *data = static_cast<std::vector<int32_t> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append((*data)[i]);
        }
        break;
      }
      case DataType::INT64: {
        auto *data = static_cast<std::vector<int64_t> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append(static_cast<Json::Int64>((*data)[i]));
        }
        break;
      }
      case DataType::FP16: {
        // FIXME(varunsh): this is not handled
        std::cout << "Writing FP16 not supported\n";
        break;
      }
      case DataType::FP32: {
        auto *data = static_cast<std::vector<float> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append((*data)[i]);
        }
        break;
      }
      case DataType::FP64: {
        auto *data = static_cast<std::vector<double> *>(output.getData());
        for (size_t i = 0; i < output.getSize(); i++) {
          json_output["data"].append((*data)[i]);
        }
        break;
      }
      case DataType::STRING: {
        auto *data = static_cast<std::string *>(output.getData());
        json_output["data"].append(*data);
        // for(size_t i = 0; i < output.getSize(); i++){
        //   json_output["data"].append(data->data()[i]);
        // }
        break;
      }
      default:
        // TODO(varunsh): what should we do here?
        std::cout << "Unknown datatype\n";
        break;
    }
    ret["outputs"].append(json_output);
  }
  return ret;
}

void drogonCallback(const DrogonCallback &callback,
                    const InferenceResponse &response) {
  drogon::HttpResponsePtr resp;
  if (response.isError()) {
    resp =
      errorHttpResponse(response.getError(), HttpStatusCode::k400BadRequest);
  } else {
    try {
      Json::Value ret = parseResponse(response);
      resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    } catch (const std::invalid_argument &e) {
      resp = errorHttpResponse(e.what(), HttpStatusCode::k400BadRequest);
    }
  }
#ifdef PROTEUS_ENABLE_TRACING
  const auto &context = response.getContext();
  propagate(resp.get(), context);
#endif
  callback(resp);
}

std::shared_ptr<InferenceRequest> DrogonHttp::getRequest(
  size_t &buffer_index, const std::vector<BufferRawPtrs> &input_buffers,
  std::vector<size_t> &input_offsets,
  const std::vector<BufferRawPtrs> &output_buffers,
  std::vector<size_t> &output_offsets, const size_t &batch_size,
  size_t &batch_offset) {
  try {
    auto request = std::make_shared<InferenceRequest>(
      this->json_, buffer_index, input_buffers, input_offsets, output_buffers,
      output_offsets, batch_size, batch_offset);
    Callback callback =
      std::bind(drogonCallback, this->callback_, std::placeholders::_1);
    request->setCallback(std::move(callback));
    return request;
  } catch (const std::invalid_argument &e) {
    SPDLOG_LOGGER_INFO(this->logger_, e.what());
    this->callback_(
      errorHttpResponse(e.what(), HttpStatusCode::k400BadRequest));
    return nullptr;
  }
}

void DrogonHttp::errorHandler(const std::invalid_argument &e) {
  SPDLOG_LOGGER_DEBUG(this->logger_, e.what());
  this->callback_(errorHttpResponse(e.what(), HttpStatusCode::k400BadRequest));
}

}  // namespace proteus::http
