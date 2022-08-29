// Copyright 2022 Xilinx Inc.
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

#include <cstddef>
#include <iostream>

#include "gtest/gtest.h"  // for Test, SuiteApiResolver, TEST
#include "proteus/client_operators/infer_async.hpp"
#include "proteus/clients/native.hpp"
#include "proteus/core/predict_api.hpp"
#include "proteus/testing/gtest_fixtures.hpp"  // for BaseFixture

namespace proteus {

TEST_F(HttpFixture, Ordered) {
  NativeClient client;
  auto endpoint = client.workerLoad("echo", nullptr);
  EXPECT_EQ(endpoint, "echo");

  std::vector<uint32_t> imgData;
  const auto shape = {1UL};
  const auto size = 1;
  imgData.reserve(size);
  imgData.push_back(1);

  proteus::InferenceRequest request;
  request.addInputTensor(imgData.data(), shape, proteus::DataType::UINT32);

  const auto data_size = 10;
  std::vector<InferenceRequest> reqs;
  reqs.reserve(data_size);
  for (auto i = 0; i < data_size; ++i) {
    reqs.push_back(request);
  }

  auto resps = inferAsyncOrdered(&client, "echo", reqs);
  EXPECT_EQ(resps.size(), data_size);
  for (const auto& resp : resps) {
    EXPECT_FALSE(resp.isError());
  }
}

}  // namespace proteus