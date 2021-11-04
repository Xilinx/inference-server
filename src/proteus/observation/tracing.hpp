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

#include <opentracing/span.h>  // IWYU pragma: export

#include <memory>  // for unique_ptr

#include "proteus/build_options.hpp"     // for PROTEUS_ENABLE_TRACING
#include "proteus/core/predict_api.hpp"  // for RequestParameters

// IWYU pragma: no_forward_declare proteus::RequestParameters

#ifdef PROTEUS_ENABLE_TRACING

namespace proteus {

using SpanPtr = std::unique_ptr<opentracing::Span>;

void startTracer(const char* configFilePath);
void stopTracer();
SpanPtr startSpan(const char* name);
SpanPtr startChildSpan(opentracing::Span* parentSpan, const char* name);
SpanPtr startFollowSpan(opentracing::Span* previousSpan, const char* name);
void setTags(opentracing::Span* span, RequestParameters* parameters);

}  // namespace proteus

#endif

#endif  // GUARD_PROTEUS_OBSERVATION_TRACING
