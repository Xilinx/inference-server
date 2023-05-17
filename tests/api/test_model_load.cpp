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

void prerequisites(const Client* client) {
  if (!serverHasExtension(client, "tfzendnn")) {
    GTEST_SKIP() << "This test requires TF+ZenDNN support.";
  }

  EXPECT_TRUE(client->modelList().empty());
}

std::string getModel() { return "mnist"; }

void testWithoutVersion(const Client* client) {
  prerequisites(client);

  const auto model = getModel();
  client->modelLoad(model, {});
  EXPECT_TRUE(client->modelReady(model));
  client->modelUnload(model);
  waitUntilModelNotReady(client, model);
}

void testWithVersion(const Client* client, const std::string& version) {
  prerequisites(client);

  const auto model = getModel();
  client->modelLoad(model, {}, version);
  EXPECT_TRUE(client->modelReady(model, version));
  client->modelUnload(model, version);
  waitUntilModelNotReady(client, model, version);
}

#ifdef AMDINFER_ENABLE_GRPC
// @pytest.mark.extensions(["tfzendnn"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, modelLoad) { testWithoutVersion(client_.get()); }

// @pytest.mark.extensions(["tfzendnn"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, modelLoadVersioned) { testWithVersion(client_.get(), "1"); }
#endif

// @pytest.mark.extensions(["tfzendnn"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, modelLoad) {
  amdinfer::NativeClient client(&server_);
  testWithoutVersion(&client);
}

// @pytest.mark.extensions(["tfzendnn"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, modelLoadVersioned) {
  amdinfer::NativeClient client(&server_);
  testWithVersion(&client, "1");
}

#ifdef AMDINFER_ENABLE_HTTP
// @pytest.mark.extensions(["tfzendnn"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, modelLoad) { testWithoutVersion(client_.get()); }

// @pytest.mark.extensions(["tfzendnn"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, modelLoadVersioned) { testWithVersion(client_.get(), "1"); }
#endif

}  // namespace amdinfer
