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

#ifndef GUARD_PROTEUS_OBSERVATION_TRACING
#define GUARD_PROTEUS_OBSERVATION_TRACING

#include <memory>  // for shared_ptr
#include <unordered_map>

#include "proteus/build_options.hpp"     // for PROTEUS_ENABLE_TRACING
#include "proteus/core/predict_api.hpp"  // for RequestParameters
#include "proteus/helpers/declarations.hpp"

// IWYU pragma: no_forward_declare proteus::RequestParameters

#ifdef PROTEUS_ENABLE_TRACING

// opentelemetry needs this definition prior to any header inclusions if the
// library was compiled with this flag set
#define HAVE_CPP_STDLIB

#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/trace/scope.h>
#include <opentelemetry/trace/span.h>  // IWYU pragma: export
#include <opentelemetry/trace/span_startoptions.h>

namespace proteus {

void startTracer();
void stopTracer();

class Trace final {
 public:
  Trace(const char* name,
        const opentelemetry::v1::trace::StartSpanOptions& options = {});
  ~Trace();
  Trace(Trace const&) = delete;              ///< Copy constructor
  Trace& operator=(const Trace&) = delete;   ///< Copy assignment constructor
  Trace(Trace&& other) = delete;             ///< Move constructor
  Trace& operator=(Trace&& other) = delete;  ///< Move assignment constructor

  // opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> getSpan()
  // const;

  void startSpan(const char* name);

  /// set an attribute in the active span in the trace
  void setAttribute(opentelemetry::nostd::string_view key,
                    const opentelemetry::common::AttributeValue& value);

  /// set all parameters as attributes in the active span in the trace
  void setAttributes(RequestParameters* parameters);

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

using TracePtr = std::unique_ptr<Trace>;

TracePtr startTrace(const char* name);
TracePtr startTrace(
  const char* name,
  const std::unordered_map<std::string, std::string>& http_headers);

}  // namespace proteus

#endif

#endif  // GUARD_PROTEUS_OBSERVATION_TRACING
