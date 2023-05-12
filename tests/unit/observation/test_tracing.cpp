// Copyright 2023 Advanced Micro Devices, Inc.
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

#include <optional>
#include <sstream>

#include "amdinfer/observation/tracing.hpp"  // for tracing
#include "gtest/gtest.h"                     // for Message, TestPartResult, ...

namespace amdinfer {

std::pair<std::string, std::string> readRow(std::stringstream& ss) {
  std::string key;
  std::string value;
  ss >> key;
  // read the colon
  ss >> value;
  // overwrite with the value
  ss >> value;

  return {key, value};
}

struct Span {
  std::string name;
  std::string trace_id;
  std::string span_id;
  std::string parent_span_id;
};

/**
 * @brief Parse the printed output of the span and extract some information from
 * it.
 * @details The output of the OStreamExporter is something like:
 *    {
 *      name          : test_2
 *      trace_id      : 3b2ddc957b2734d89d49b5eff44ca5a0
 *      span_id       : 8685ded53f402c8a
 *      tracestate    :
 *      parent_span_id: 35e48e996f3f6a70
 *      start         : 1683064180320834800
 *      duration      : 3306
 *      description   :
 *      span kind     : Internal
 *      status        : Unset
 *      attributes    :
 *      events        :
 *      links         :
 *      resources     :
 *            telemetry.sdk.version: 1.1.0
 *            telemetry.sdk.name: opentelemetry
 *            telemetry.sdk.language: cpp
 *            service.name: amdinfer
 *      instr-lib     : amdinfer
 *    }
 *  This output, while close to JSON, is not JSON and has to be manually parsed.
 *
 * @param ss stringstream to read from
 * @return std::optional<Span> returns a Span if successfully parsed
 */
std::optional<Span> readStream(std::stringstream& ss) {
  std::string span_str;
  ss >> span_str;  // {

  auto [name_key, name_value] = readRow(ss);
  if (name_key != "name") {
    return {};
  }

  Span span;
  span.name = name_value;

  auto [trace_id_key, trace_id_value] = readRow(ss);
  if (trace_id_key != "trace_id") {
    return {};
  }
  span.trace_id = trace_id_value;

  auto [span_id_key, span_id_value] = readRow(ss);
  if (span_id_key != "span_id") {
    return {};
  }
  span.span_id = span_id_value;

  ss >> span_str;  // tracestate
  ss >> span_str;  // :

  ss >> span_str;
  if (span_str != "parent_span_id:") {
    return {};
  }

  ss >> span.parent_span_id;

  // clear the stream
  ss.str("");

  return span;
}

#ifdef AMDINFER_ENABLE_TRACING
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitTracing, Basic) {
  std::stringstream ss;

  startOStreamTracer(ss);

  std::string outer{"outer"};
  auto trace = startTrace(outer.c_str());

  std::string inner{"inner"};
  trace->startSpan(inner.c_str());
  trace->endSpan();

  auto maybe_span = readStream(ss);
  ASSERT_TRUE(maybe_span);
  const auto& span = maybe_span.value();
  ASSERT_EQ(span.name, inner);

  trace->endTrace();

  auto maybe_span_2 = readStream(ss);
  ASSERT_TRUE(maybe_span_2);
  const auto& span_2 = maybe_span_2.value();

  ASSERT_EQ(span_2.name, outer);
  ASSERT_EQ(span.trace_id, span_2.trace_id);
  ASSERT_NE(span.span_id, span_2.span_id);
  ASSERT_EQ(span.parent_span_id, span_2.span_id);

  stopTracer();
}

TEST(UnitTracing, AutoDestroy) {
  std::stringstream ss;

  startOStreamTracer(ss);
  { auto trace = startTrace("test"); }

  stopTracer();
}
#endif

}  // namespace amdinfer
