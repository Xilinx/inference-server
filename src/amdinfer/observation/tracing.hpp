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
 * @brief Defines tracing
 */

#ifndef GUARD_AMDINFER_OBSERVATION_TRACING
#define GUARD_AMDINFER_OBSERVATION_TRACING

#include <iostream>  // for cout
#include <memory>    // for shared_ptr, uniqu...
#include <stack>     // for stack

#include "amdinfer/build_options.hpp"    // for AMDINFER_ENABLE_TR...
#include "amdinfer/core/parameters.hpp"  // for ParameterMap
#include "amdinfer/declarations.hpp"     // for StringMap

// IWYU pragma: no_forward_declare amdinfer::ParameterMap

#ifdef AMDINFER_ENABLE_TRACING

// opentelemetry needs this definition prior to any header inclusions if the
// library was compiled with this flag set, which it is in the Docker build
#define HAVE_CPP_STDLIB

#include <opentelemetry/common/attribute_value.h>  // for AttributeValue
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/std/string_view.h>          // for string_view
#include <opentelemetry/trace/span.h>               // for span
#include <opentelemetry/trace/span_startoptions.h>  // for StartSpanOptions

namespace amdinfer {

// only initialize one exporter

/// initialize tracing globally using ostream exporter
void startOStreamTracer(std::ostream& os = std::cout);
/// initialize tracing globally using Jaeger exporter
void startJaegerTracer();
/// clean up the tracing prior to shutdown
void stopTracer();

/**
 * @brief The Trace object abstracts the details of how tracing is implemented.
 * This object should not be created directly (use startTrace()).
 *
 */
class Trace final {
 public:
  explicit Trace(
    const char* name,
    const opentelemetry::v1::trace::StartSpanOptions& options = {});
  ~Trace();
  Trace(Trace const&) = delete;              ///< Copy constructor
  Trace& operator=(const Trace&) = delete;   ///< Copy assignment constructor
  Trace(Trace&& other) = delete;             ///< Move constructor
  Trace& operator=(Trace&& other) = delete;  ///< Move assignment constructor

  /// start a new span with the given name
  void startSpan(const char* name);

  /// set an attribute in the active span in the trace
  void setAttribute(opentelemetry::nostd::string_view key,
                    const opentelemetry::common::AttributeValue& value);

  /// set all parameters as attributes in the active span in the trace
  void setAttributes(const ParameterMap& parameters);

  /// ends all spans except the last one and returns its context for propagation
  StringMap propagate();

  /// Ends the active span in the trace
  void endSpan();

  /// Ends the trace
  void endTrace();

 private:
  std::stack<opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span>>
    spans_;
  // std::unique_ptr<opentelemetry::trace::Scope> scope_;
};

/// Start a trace with the given name
TracePtr startTrace(const char* name);

/**
 * @brief Start a trace with context from HTTP headers
 *
 * @param name name of the trace
 * @param http_headers headers from HTTP request to extract context from
 * @return TracePtr
 */
TracePtr startTrace(const char* name, const StringMap& http_headers);

}  // namespace amdinfer

#endif

#endif  // GUARD_AMDINFER_OBSERVATION_TRACING
