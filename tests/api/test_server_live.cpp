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

#include <memory>  // for allocator, unique_ptr

#include "proteus/proteus.hpp"                 // for GrpcClient
#include "proteus/testing/gtest_fixtures.hpp"  // for GrpcFixture

void test(proteus::Client* client) { EXPECT_TRUE(client->serverLive()); }

#ifdef PROTEUS_ENABLE_GRPC
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, ServerLive) { test(client_.get()); }
#endif

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, ServerLive) {
  proteus::NativeClient client;
  test(&client);
}

#ifdef PROTEUS_ENABLE_HTTP
TEST_F(HttpFixture, ServerLive) { test(client_.get()); }
#endif
