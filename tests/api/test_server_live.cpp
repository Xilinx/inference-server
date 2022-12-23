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

#include <memory>  // for allocator, unique_ptr

#include "amdinfer/amdinfer.hpp"                // for GrpcClient
#include "amdinfer/testing/gtest_fixtures.hpp"  // for GrpcFixture

void test(amdinfer::Client* client) { EXPECT_TRUE(client->serverLive()); }

#ifdef AMDINFER_ENABLE_GRPC
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, ServerLive) { test(client_.get()); }
#endif

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, ServerLive) {
  amdinfer::NativeClient client;
  test(&client);
}

#ifdef AMDINFER_ENABLE_HTTP
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, ServerLive) { test(client_.get()); }
#endif
