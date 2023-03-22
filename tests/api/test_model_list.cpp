// Copyright 2022 Xilinx, Inc.
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

#include <algorithm>  // for find
#include <chrono>     // for milliseconds
#include <memory>     // for allocator, unique_ptr
#include <string>     // for string, basic_string
#include <thread>     // for sleep_for
#include <vector>     // for vector

#include "amdinfer/amdinfer.hpp"                // for NativeClient, GrpcClient
#include "amdinfer/testing/gtest_fixtures.hpp"  // for AssertionResult, Message

namespace amdinfer {

void test(const Client* client) {
  const std::string worker = "cplusplus";

  auto models_0 = client->modelList();
  EXPECT_TRUE(models_0.empty());

  ParameterMap parameters_0;
  parameters_0.put("model", "echo");
  const auto endpoint = client->workerLoad(worker, parameters_0);
  EXPECT_EQ(endpoint, worker);
  EXPECT_TRUE(client->modelReady(endpoint));

  auto models = client->modelList();
  EXPECT_EQ(models.size(), 1);
  EXPECT_EQ(models.at(0), endpoint);

  ParameterMap parameters_2;
  parameters_2.put("model", "echo_multi");
  const auto endpoint_2 = client->workerLoad(worker, parameters_2);
  EXPECT_TRUE(client->modelReady(endpoint_2));

  auto models_2 = client->modelList();
  EXPECT_EQ(models_2.size(), 2);
  EXPECT_TRUE(std::find(models_2.begin(), models_2.end(), endpoint) !=
              models.end());
  EXPECT_TRUE(std::find(models_2.begin(), models_2.end(), endpoint_2) !=
              models.end());

  client->modelUnload(endpoint);
  client->modelUnload(endpoint_2);

  auto models_3 = client->modelList();
  while (!models_3.empty()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    models_3 = client->modelList();
  }
}

#ifdef AMDINFER_ENABLE_GRPC
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, ModelList) { test(client_.get()); }
#endif

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, ModelList) {
  NativeClient client(&server_);
  test(&client);
}

#ifdef AMDINFER_ENABLE_HTTP
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, ModelList) { test(client_.get()); }
#endif

}  // namespace amdinfer
