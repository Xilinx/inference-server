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

bool isReady(proteus::Client* client, const std::string& endpoint) {
  try {
    return client->modelReady(endpoint);
  } catch (const std::invalid_argument& e) {
    return false;
  }
}

void test(proteus::Client* client) {
  const std::string worker = "echo";

  auto models_0 = client->modelList();
  EXPECT_TRUE(models_0.empty());

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto, hicpp-avoid-goto)
  EXPECT_THROW_CHECK({ client->modelReady(worker); },
                     { EXPECT_STREQ("worker echo not found", e.what()); },
                     std::invalid_argument);

  const auto endpoint = client->modelLoad(worker, nullptr);
  EXPECT_EQ(endpoint, worker);

  while (!isReady(client, endpoint)) {
    std::this_thread::yield();
  }

  auto models = client->modelList();
  EXPECT_EQ(models.size(), 1);

  client->modelUnload(endpoint);

  while (isReady(client, endpoint)) {
    std::this_thread::yield();
  }

  while (!client->modelList().empty()) {
    std::this_thread::yield();
  }
}

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, ModelReady) { test(client_.get()); }

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, ModelReady) {
  proteus::NativeClient client;
  test(&client);
}

TEST_F(HttpFixture, ModelReady) { test(client_.get()); }
