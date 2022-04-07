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

#include "proteus/clients/client.hpp"          // for Client
#include "proteus/proteus.hpp"                 // for GrpcClient
#include "proteus/testing/gtest_fixtures.hpp"  // for AssertionResult, Suite...
#include "proteus/testing/native.hpp"          // for NativeClient

void test(proteus::Client* client) {
  std::string worker = "echo";

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto, hicpp-avoid-goto)
  EXPECT_THROW_CHECK({ client->modelReady(worker); },
                     { EXPECT_STREQ("worker echo not found", e.what()); },
                     std::invalid_argument);

  auto endpoint = client->modelLoad(worker, nullptr);
  EXPECT_EQ(endpoint, worker);

  EXPECT_TRUE(client->modelReady(endpoint));
}

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, ModelLoad) { test(client_.get()); }

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, ModelLoad) {
  proteus::NativeClient client;
  test(&client);
}
