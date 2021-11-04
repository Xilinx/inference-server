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

#include <jaegertracing/Config.h>   // for Config
#include <jaegertracing/Logging.h>  // for consoleLogger, Logger
#include <jaegertracing/Tracer.h>   // for Tracer
#include <opentracing/tracer.h>     // for Tracer, ChildOf, FollowsFrom
#include <yaml-cpp/yaml.h>          // for LoadFile

#include <cstdint>  // for int32_t
#include <map>      // for _Rb_tree_iterator, operator!=
#include <string>   // for string, basic_string
#include <utility>  // for pair
#include <variant>  // for get, holds_alternative, visi

#include "proteus/core/predict_api.hpp"  // for Parameter, RequestParameters

#ifdef PROTEUS_ENABLE_TRACING

namespace proteus {

void startTracer(const char* configFilePath) {
  auto configYAML = YAML::LoadFile(configFilePath);
  auto config = jaegertracing::Config::parse(configYAML);
  auto tracer = jaegertracing::Tracer::make(
    "proteus", config, jaegertracing::logging::consoleLogger());
  opentracing::Tracer::InitGlobal(
    std::static_pointer_cast<opentracing::Tracer>(tracer));
}

void stopTracer() { opentracing::Tracer::Global()->Close(); }

SpanPtr startSpan(const char* name) {
  return opentracing::Tracer::Global()->StartSpan(name);
}

SpanPtr startChildSpan(opentracing::Span* parentSpan, const char* name) {
  return opentracing::Tracer::Global()->StartSpan(
    name, {opentracing::ChildOf(&(parentSpan->context()))});
}

SpanPtr startFollowSpan(opentracing::Span* previousSpan, const char* name) {
  return opentracing::Tracer::Global()->StartSpan(
    name, {opentracing::FollowsFrom(&(previousSpan->context()))});
}

void setTags(opentracing::Span* span, RequestParameters* parameters) {
  auto data = parameters->data();
  // a range-based for loop doesn't work here because we can't pass the key when
  // it's a structured binding.
  for (auto it = data.begin(); it != data.end(); it++) {
    auto key = it->first;
    auto value = it->second;
    std::visit(
      [key, &span](Parameter&& arg) {
        if (std::holds_alternative<bool>(arg))
          span->SetTag(key, std::get<bool>(arg));
        else if (std::holds_alternative<double>(arg))
          span->SetTag(key, std::get<double>(arg));
        else if (std::holds_alternative<int32_t>(arg))
          span->SetTag(key, std::get<int32_t>(arg));
        else if (std::holds_alternative<std::string>(arg))
          span->SetTag(key, std::get<std::string>(arg));
      },
      value);
  }
}

}  // namespace proteus

#endif
