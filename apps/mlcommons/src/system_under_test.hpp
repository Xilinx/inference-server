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

#ifndef GUARD_MLCOMMONS_SRC_SYSTEM_UNDER_TEST
#define GUARD_MLCOMMONS_SRC_SYSTEM_UNDER_TEST

#include <concurrentqueue/blockingconcurrentqueue.h>
#include <mlcommons/loadgen/system_under_test.h>

#include "amdinfer/amdinfer.hpp"

namespace amdinfer {

class QuerySampleLibrary;

class SystemUnderTest : public mlperf::SystemUnderTest {
 public:
  SystemUnderTest(QuerySampleLibrary* qsl, Client* client,
                  const std::string& endpoint);

  // void execBatch(std::vector<mlperf::QuerySample> batch);

  ~SystemUnderTest() noexcept override = default;

  const std::string& Name() const override;

  void IssueQuery(const std::vector<mlperf::QuerySample>& samples) override;
  void FinishQuery();

  void FlushQueries() override;

  void ReportLatencyResults(
    const std::vector<mlperf::QuerySampleLatency>& latencies_ns) override;

 private:
  std::string name_{"AMD Inference Server"};
  QuerySampleLibrary* qsl_;
  Client* client_;
  std::string endpoint_;
  moodycamel::BlockingConcurrentQueue<InferenceResponseFuture> queue_;
};

}  // namespace amdinfer

#endif  // GUARD_MLCOMMONS_SRC_SYSTEM_UNDER_TEST
