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

#include "proteus/observation/tracing.hpp"

#include <opentelemetry/context/propagation/global_propagator.h>
#include <opentelemetry/context/propagation/text_map_propagator.h>
#include <opentelemetry/exporters/jaeger/jaeger_exporter.h>
#include <opentelemetry/exporters/ostream/span_exporter.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>
#include <opentelemetry/trace/provider.h>

#include <cstdint>  // for int32_t
#include <map>      // for _Rb_tree_iterator, operator!=
#include <string>   // for string, basic_string
#include <utility>  // for pair
#include <variant>  // for get, holds_alternative, visi

#include "proteus/core/predict_api.hpp"  // for Parameter, RequestParameters

#ifdef PROTEUS_ENABLE_TRACING

namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace nostd = opentelemetry::nostd;

namespace proteus {

/**
 * @brief This class provides the interface to hold the HTTP context from
 * incoming requests so we can propogate it. This class is taken from the HTTP
 * example in opentelemetry.
 *
 */
class HttpTextMapCarrier
  : public opentelemetry::context::propagation::TextMapCarrier {
 public:
  HttpTextMapCarrier(const StringMap& headers) : headers_(headers) {}
  HttpTextMapCarrier() = default;
  virtual opentelemetry::nostd::string_view Get(
    opentelemetry::nostd::string_view key) const noexcept override {
    auto it = headers_.find(key.data());
    if (it != headers_.end()) {
      return it->second;
    }
    return "";
  }

  virtual void Set(opentelemetry::nostd::string_view key,
                   opentelemetry::nostd::string_view value) noexcept override {
    headers_.insert(std::pair<std::string, std::string>(std::string(key),
                                                        std::string(value)));
  }

  StringMap headers_;
};

void startTracer() {
  /**
   * Using "new" here instead of make_unique because g++ complains during
   * compilation about the destructor of the unique_ptr using an incomplete
   * type ThriftSender. This is forward-declared in jaeger_exporter.h and
   * already statically-linked into opentelemetry. We'd need to include Thrift
   * headers to use make_unique
   */
  // auto exporter  = std::unique_ptr<trace_sdk::SpanExporter>(new
  //   opentelemetry::exporter::trace::OStreamSpanExporter());
  auto exporter = std::unique_ptr<trace_sdk::SpanExporter>(
    new opentelemetry::exporter::jaeger::JaegerExporter());
  auto processor =
    std::make_unique<trace_sdk::SimpleSpanProcessor>(std::move(exporter));
  auto provider =
    nostd::shared_ptr<trace_api::TracerProvider>(new trace_sdk::TracerProvider(
      std::move(processor), opentelemetry::sdk::resource::Resource::Create(
                              {{"service.name", "proteus"}})));

  auto propagator =
    nostd::shared_ptr<opentelemetry::context::propagation::TextMapPropagator>(
      new opentelemetry::trace::propagation::HttpTraceContext());

  // Set the global trace provider
  trace_api::Provider::SetTracerProvider(provider);
  // set global propagator
  opentelemetry::context::propagation::GlobalTextMapPropagator::
    SetGlobalPropagator(propagator);
}

nostd::shared_ptr<trace_api::Tracer> get_tracer() {
  auto provider = trace_api::Provider::GetTracerProvider();
  return provider->GetTracer("proteus");
}

void stopTracer() {
  auto tracer = get_tracer();
  tracer->Close(std::chrono::milliseconds(1));
}

Trace::Trace(const char* name,
             const opentelemetry::v1::trace::StartSpanOptions& options) {
  auto tracer = get_tracer();
  this->spans_.emplace(tracer->StartSpan(name, options));
}

Trace::~Trace() { this->endTrace(); }

void Trace::startSpan(const char* name) {
  auto scope = trace_api::Scope(this->spans_.top());  // mark last span active
  auto tracer = get_tracer();
  this->spans_.emplace(tracer->StartSpan(name));
}

void Trace::setAttribute(nostd::string_view key,
                         const opentelemetry::common::AttributeValue& value) {
  this->spans_.top()->SetAttribute(key, value);
}

void Trace::setAttributes(RequestParameters* parameters) {
  auto data = parameters->data();
  // a range-based for loop doesn't work here because we can't pass the key when
  // it's a structured binding.
  for (auto it = data.begin(); it != data.end(); it++) {
    auto key = it->first;
    auto value = it->second;
    std::visit(
      [key, this](Parameter&& arg) {
        if (std::holds_alternative<bool>(arg))
          this->spans_.top()->SetAttribute(key, std::get<bool>(arg));
        else if (std::holds_alternative<double>(arg))
          this->spans_.top()->SetAttribute(key, std::get<double>(arg));
        else if (std::holds_alternative<int32_t>(arg))
          this->spans_.top()->SetAttribute(key, std::get<int32_t>(arg));
        else if (std::holds_alternative<std::string>(arg))
          this->spans_.top()->SetAttribute(key, std::get<std::string>(arg));
      },
      value);
  }
}

void Trace::endSpan() {
  this->spans_.top()->End();
  this->spans_.pop();
}

StringMap Trace::propagate() {
  while (this->spans_.size() > 1) {
    this->endSpan();
  }
  auto scope = trace_api::Scope(this->spans_.top());  // mark last span active
  this->spans_.top()->End();

  auto current_ctx = opentelemetry::context::RuntimeContext::GetCurrent();
  HttpTextMapCarrier carrier;
  auto prop = opentelemetry::context::propagation::GlobalTextMapPropagator::
    GetGlobalPropagator();
  prop->Inject(carrier, current_ctx);
  return carrier.headers_;
}

void Trace::endTrace() {
  while (!this->spans_.empty()) {
    this->endSpan();
  }
}

TracePtr startTrace(const char* name) { return std::make_unique<Trace>(name); }

TracePtr startTrace(
  const char* name,
  const std::unordered_map<std::string, std::string>& http_headers) {
  auto prop = opentelemetry::context::propagation::GlobalTextMapPropagator::
    GetGlobalPropagator();
  auto current_ctx = opentelemetry::context::RuntimeContext::GetCurrent();

  const HttpTextMapCarrier carrier(http_headers);
  auto new_context = prop->Extract(carrier, current_ctx);

  trace_api::StartSpanOptions options;
  options.kind = trace_api::SpanKind::kServer;
  options.parent = trace_api::GetSpan(new_context)->GetContext();

  return std::make_unique<Trace>(name, options);
}

}  // namespace proteus

#endif
