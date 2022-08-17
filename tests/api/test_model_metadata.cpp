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

#include <chrono>  // for seconds
#include <memory>  // for unique_ptr
#include <string>  // for allocator, string
#include <thread>  // for sleep_for
#include <vector>  // for vector

#include "proteus/proteus.hpp"                 // for ModelMetadata, NativeC...
#include "proteus/testing/gtest_fixtures.hpp"  // for AssertionResult, Suite...

void test(proteus::Client* client) {
  const std::string model = "echo";

  EXPECT_TRUE(client->modelList().empty());

  // load one worker
  client->workerLoad(model, nullptr);

  EXPECT_TRUE(client->modelReady(model));

  auto metadata = client->modelMetadata(model);
  EXPECT_EQ(metadata.getName(), "echo");
  EXPECT_EQ(metadata.getPlatform(), "cpu");
  // TODO(varunsh): add other assertions

  client->workerUnload(model);  // unload the model

  while (!client->modelList().empty()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

#ifdef PROTEUS_ENABLE_GRPC
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, modelMetadata) { test(client_.get()); }
#endif

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, modelMetadata) {
  proteus::NativeClient client;
  test(&client);
}

#ifdef PROTEUS_ENABLE_HTTP
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, modelMetadata) { test(client_.get()); }
#endif
