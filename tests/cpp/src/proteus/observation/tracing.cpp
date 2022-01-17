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
 * @brief Implements tracing in Proteus
 */

#include "proteus/observation/tracing.hpp"

#include <opentelemetry/context/propagation/global_propagator.h>
#include <opentelemetry/context/propagation/text_map_propagator.h>
#include <opentelemetry/context/runtime_context.h>
#include <opentelemetry/exporters/jaeger/jaeger_exporter.h>
// #include <opentelemetry/exporters/ostream/span_exporter.h>
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

void startTracer() {}

nostd::shared_ptr<trace_api::Tracer> get_tracer() { return nullptr; }

void stopTracer() {}

Trace::Trace(const char* name,
             const opentelemetry::v1::trace::StartSpanOptions& options) {
  (void)name;
  (void)options;
}

Trace::~Trace() {}

void Trace::startSpan(const char* name) { (void)name; }

void Trace::setAttribute(nostd::string_view key,
                         const opentelemetry::common::AttributeValue& value) {
  (void)key;
  (void)value;
}

void Trace::setAttributes(RequestParameters* parameters) { (void)parameters; }

void Trace::endSpan() {}

StringMap Trace::propagate() { return StringMap(); }

void Trace::endTrace() {}

TracePtr startTrace(const char* name) { return std::make_unique<Trace>(name); }

TracePtr startTrace(const char* name, const StringMap& http_headers) {
  (void)http_headers;
  return std::make_unique<Trace>(name);
}

}  // namespace proteus

#endif
