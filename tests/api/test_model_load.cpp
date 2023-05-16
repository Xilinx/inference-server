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

#include <memory>         // for allocator, unique_ptr
#include <string>         // for string
#include <thread>         // for yield
#include <unordered_set>  // for operator==, unordered...
#include <vector>         // for vector

#include "amdinfer/amdinfer.hpp"                // for NativeClient, GrpcClient
#include "amdinfer/testing/gtest_fixtures.hpp"  // for AssertionResult, Suite...

namespace amdinfer {

void test(const amdinfer::Client* client) {
  if (!serverHasExtension(client, "tfzendnn")) {
    GTEST_SKIP() << "This test requires TF+ZenDNN support.";
  }

  const std::string model = "mnist";

  EXPECT_TRUE(client->modelList().empty());

  // load one worker
  client->modelLoad(model, {});

  EXPECT_TRUE(client->modelReady(model));

  client->modelUnload(model);  // unload the model

  waitUntilModelNotReady(client, model);
}

#ifdef AMDINFER_ENABLE_GRPC
// @pytest.mark.extensions(["tfzendnn"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, modelLoad) { test(client_.get()); }
#endif

// @pytest.mark.extensions(["tfzendnn"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, modelLoad) {
  amdinfer::NativeClient client(&server_);
  test(&client);
}

#ifdef AMDINFER_ENABLE_HTTP
// @pytest.mark.extensions(["tfzendnn"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, modelLoad) { test(client_.get()); }
#endif

}  // namespace amdinfer
