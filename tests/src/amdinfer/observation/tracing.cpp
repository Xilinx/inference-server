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
 * @brief Implements tracing
 */

#include "amdinfer/observation/tracing.hpp"

#include <opentelemetry/std/utility.h>           // for nostd
#include <opentelemetry/trace/canonical_code.h>  // for trace
#include <opentelemetry/trace/tracer.h>          // for Tracer

#ifdef AMDINFER_ENABLE_TRACING

namespace trace_api = opentelemetry::trace;
namespace nostd = opentelemetry::nostd;

namespace amdinfer {

void startTracer() {}

nostd::shared_ptr<trace_api::Tracer> get_tracer() { return nullptr; }

void stopTracer() {}

Trace::Trace(const char* name,
             const opentelemetry::v1::trace::StartSpanOptions& options) {
  (void)name;
  (void)options;
}

Trace::~Trace() = default;

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

}  // namespace amdinfer

#endif
