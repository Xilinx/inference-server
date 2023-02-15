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

/**
 * @file
 * @brief
 */

#include "system_under_test.hpp"

#include <mlcommons/loadgen/loadgen.h>

#include <cassert>

#include "query_sample_library.hpp"

namespace amdinfer {

const std::string& SystemUnderTestNative::Name() const { return name_; }

SystemUnderTestNative::SystemUnderTestNative(QuerySampleLibrary* qsl,
                                             const std::string& model,
                                             ParameterMap parameters)
  : qsl_(qsl), client_(&server_) {
  waitUntilServerReady(&client_);
  endpoint_ = client_.workerLoad(model, &parameters);
  waitUntilModelReady(&client_, endpoint_);
}

void SystemUnderTestNative::IssueQuery(
  const std::vector<mlperf::QuerySample>& samples) {
  for (const auto& sample : samples) {
    auto& request = qsl_->getSample(sample.index);
    request.setID(std::to_string(sample.id));
    auto response = client_.modelInfer(endpoint_, request);
    assert(!response.isError());
    const auto& outputs = response.getOutputs();
    assert(outputs.size() == 1);
    auto& output = outputs[0];
    auto data = reinterpret_cast<uintptr_t>(output.getData());
    mlperf::QuerySampleResponse result{sample.id, data, output.getSize()};
    mlperf::QuerySamplesComplete(&result, 1);
  }
}

void SystemUnderTestNative::FlushQueries() {}

void SystemUnderTestNative::ReportLatencyResults(
  [[maybe_unused]] const std::vector<mlperf::QuerySampleLatency>&
    latencies_ns) {}

}  // namespace amdinfer
