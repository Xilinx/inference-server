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

#include <memory>     // for allocator, unique_ptr
#include <stdexcept>  // for runtime_error
#include <string>     // for string

#include "proteus/proteus.hpp"                 // for GrpcClient, NativeClient
#include "proteus/testing/gtest_fixtures.hpp"  // for AssertionResult, Suite...

void test(proteus::Client* client) {
  const std::string model = "mnist";

  EXPECT_TRUE(client->modelList().empty());

  // load one worker
  client->modelLoad(model, nullptr);

  EXPECT_TRUE(client->modelReady(model));

  client->modelUnload(model);  // unload the model

  while (!client->modelList().empty()) {
    std::this_thread::yield();
  }
}

#ifdef PROTEUS_ENABLE_GRPC
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, workerLoad) { test(client_.get()); }
#endif

// // NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
// TEST_F(BaseFixture, workerLoad) {
//   proteus::NativeClient client;
//   test(&client);
// }

#ifdef PROTEUS_ENABLE_HTTP
TEST_F(HttpFixture, modelLoad) { test(client_.get()); }
#endif
