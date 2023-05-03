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
 * @brief Implements tracing
 */

#include "amdinfer/observation/tracing.hpp"

#include <opentelemetry/context/propagation/global_propagator.h>
#include <opentelemetry/context/propagation/text_map_propagator.h>
#include <opentelemetry/context/runtime_context.h>
#include <opentelemetry/exporters/ostream/span_exporter.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/sdk/trace/exporter.h>
#include <opentelemetry/sdk/trace/processor.h>
#include <opentelemetry/sdk/trace/recordable.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/std/utility.h>
#include <opentelemetry/trace/canonical_code.h>
#include <opentelemetry/trace/context.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/scope.h>
#include <opentelemetry/trace/span_metadata.h>
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/trace/tracer_provider.h>

#include <chrono>
#include <cstdint>
#include <ext/alloc_traits.h>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>  // for move
#include <variant>  // for get

#ifdef AMDINFER_ENABLE_TRACING

namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace nostd = opentelemetry::nostd;

namespace amdinfer {

/**
 * @brief This class provides the interface to hold the HTTP context from
 * incoming requests so we can propagate it. This class is taken from the HTTP
 * example in opentelemetry.
 *
 */
class HttpTextMapCarrier
  : public opentelemetry::context::propagation::TextMapCarrier {
 public:
  explicit HttpTextMapCarrier(StringMap headers)
    : headers_(std::move(headers)) {}
  HttpTextMapCarrier() = default;
  std::string_view Get(std::string_view key) const noexcept override {
    if (auto it = headers_.find(key.data()); it != headers_.end()) {
      return it->second;
    }
    return "";
  }

  void Set(std::string_view key, std::string_view value) noexcept override {
    headers_.try_emplace(std::string(key), value);
  }

  [[nodiscard]] StringMap getHeaders() const&& { return headers_; }

  [[nodiscard]] const StringMap& getHeaders() const& { return headers_; }

 private:
  StringMap headers_;
};

void startTracer(std::unique_ptr<trace_sdk::SpanExporter> exporter) {
  auto processor =
    std::make_unique<trace_sdk::SimpleSpanProcessor>(std::move(exporter));
  auto provider = std::make_shared<trace_sdk::TracerProvider>(
    std::move(processor), opentelemetry::sdk::resource::Resource::Create(
                            {{"service.name", "amdinfer"}}));

  auto propagator =
    std::make_shared<opentelemetry::trace::propagation::HttpTraceContext>();

  // Set the global trace provider
  trace_api::Provider::SetTracerProvider(provider);
  // set global propagator
  opentelemetry::context::propagation::GlobalTextMapPropagator::
    SetGlobalPropagator(propagator);
}

void startOStreamTracer(std::ostream& os) {
  auto exporter =
    std::make_unique<opentelemetry::exporter::trace::OStreamSpanExporter>(os);

  startTracer(std::move(exporter));
}

void startOtlpTracer() {
  auto exporter =
    std::make_unique<opentelemetry::exporter::otlp::OtlpHttpExporter>();

  startTracer(std::move(exporter));
}

nostd::shared_ptr<trace_api::Tracer> getTracer() {
  auto provider = trace_api::Provider::GetTracerProvider();
  return provider->GetTracer("amdinfer");
}

void stopTracer() {
  auto tracer = getTracer();
  tracer->Close(std::chrono::milliseconds(1));
}

Trace::Trace(const std::string& name,
             const opentelemetry::v1::trace::StartSpanOptions& options) {
  auto tracer = getTracer();
  this->spans_.emplace(tracer->StartSpan(name, options));
}

Trace::~Trace() { this->endTrace(); }

void Trace::startSpan(const std::string& name) {
  auto scope = trace_api::Scope(this->spans_.top());  // mark last span active
  auto tracer = getTracer();
  this->spans_.emplace(tracer->StartSpan(name));
}

void Trace::setAttribute(nostd::string_view key,
                         const opentelemetry::common::AttributeValue& value) {
  this->spans_.top()->SetAttribute(key, value);
}

void Trace::setAttributes(const ParameterMap& parameters) {
  auto data = parameters.data();
  // a range-based for loop doesn't work here because we can't pass the key when
  // it's a structured binding.
  for (const auto& it : data) {
    const auto& key = it.first;
    const auto& value = it.second;
    std::visit(
      [key, this](const Parameter& arg) {
        if (std::holds_alternative<bool>(arg)) {
          this->spans_.top()->SetAttribute(key, std::get<bool>(arg));
        } else if (std::holds_alternative<double>(arg)) {
          this->spans_.top()->SetAttribute(key, std::get<double>(arg));
        } else if (std::holds_alternative<int32_t>(arg)) {
          this->spans_.top()->SetAttribute(key, std::get<int32_t>(arg));
        } else if (std::holds_alternative<std::string>(arg)) {
          this->spans_.top()->SetAttribute(key, std::get<std::string>(arg));
        }
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
  return carrier.getHeaders();
}

void Trace::endTrace() {
  while (!this->spans_.empty()) {
    this->endSpan();
  }
}

TracePtr startTrace(const std::string& name) {
  return std::make_unique<Trace>(name);
}

TracePtr startTrace(const std::string& name, const StringMap& http_headers) {
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

}  // namespace amdinfer

#endif
