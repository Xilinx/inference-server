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

#include <algorithm>  // for find
#include <memory>     // for allocator, unique_ptr
#include <stdexcept>  // for runtime_error
#include <string>     // for string

#include "proteus/proteus.hpp"                 // for GrpcClient, NativeClient
#include "proteus/testing/gtest_fixtures.hpp"  // for AssertionResult, Suite...

void test(proteus::Client* client) {
  const std::string worker = "echo";

  auto models_0 = client->modelList();
  EXPECT_EQ(models_0.size(), 0);

  const auto endpoint = client->modelLoad(worker, nullptr);
  EXPECT_EQ(endpoint, worker);
  EXPECT_TRUE(client->modelReady(endpoint));

  auto models = client->modelList();
  EXPECT_EQ(models.size(), 1);
  EXPECT_EQ(models.at(0), endpoint);

  const std::string worker_2 = "InvertImage";
  const auto endpoint_2 = client->modelLoad(worker_2, nullptr);
  EXPECT_EQ(endpoint_2, worker_2);
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
  while (models_3.size() > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    models_3 = client->modelList();
  }
}

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, ModelList) { test(client_.get()); }

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, ModelList) {
  proteus::NativeClient client;
  test(&client);
}
