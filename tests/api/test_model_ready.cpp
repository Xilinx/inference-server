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

#include <chrono>  // for milliseconds
#include <memory>  // for allocator, unique_ptr
#include <string>  // for string
#include <thread>  // for sleep_for, yield
#include <vector>  // for vector

#include "amdinfer/amdinfer.hpp"                // for NativeClient, GrpcClient
#include "amdinfer/testing/gtest_fixtures.hpp"  // for AssertionResult, Suite...

namespace amdinfer {

bool isReady(const Client* client, const std::string& endpoint) {
  try {
    return client->modelReady(endpoint);
  } catch (const bad_status&) {
    return false;
  }
}

void test(const Client* client) {
  const std::string worker = "cplusplus";

  auto models_0 = client->modelList();
  EXPECT_TRUE(models_0.empty());

  EXPECT_FALSE(client->modelReady(worker));

  const auto endpoint = client->workerLoad(worker, {{"model"}, {"echo"}});
  EXPECT_EQ(endpoint, worker);

  // arbitrarily set to 10ms
  const auto delay = std::chrono::milliseconds(10);

  while (!client->modelReady(endpoint)) {
    std::this_thread::sleep_for(delay);
  }

  auto models = client->modelList();
  EXPECT_EQ(models.size(), 1);

  client->modelUnload(endpoint);

  while (isReady(client, endpoint)) {
    std::this_thread::sleep_for(delay);
  }

  while (!client->modelList().empty()) {
    std::this_thread::yield();
  }
}

#ifdef AMDINFER_ENABLE_GRPC
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, ModelReady) { test(client_.get()); }
#endif

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, ModelReady) {
  NativeClient client(&server_);
  test(&client);
}

#ifdef AMDINFER_ENABLE_HTTP
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, ModelReady) { test(client_.get()); }
#endif

}  // namespace amdinfer
