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
#include <string>  // for basic_string, string
#include <thread>  // for yield
#include <vector>  // for vector

#include "amdinfer/amdinfer.hpp"                // for Client, RequestParame...
#include "amdinfer/testing/gtest_fixtures.hpp"  // for AssertionResult, Message

void test(amdinfer::Client* client) {
  std::string worker = "echo";

  EXPECT_TRUE(client->modelList().empty());

  // load one worker
  auto endpoint = client->workerLoad(worker, nullptr);
  EXPECT_EQ(endpoint, worker);
  // do a redundant load
  endpoint = client->workerLoad(worker, nullptr);
  EXPECT_EQ(endpoint, worker);

  // load the same worker with a different config
  amdinfer::ParameterMap parameters;
  // arbitrarily set to 100 just to create a different config
  const auto max_buffer_num = 100;
  parameters.put("max_buffer_num", max_buffer_num);
  auto endpoint_1 = client->workerLoad(worker, &parameters);
  EXPECT_EQ(endpoint_1, "echo-0");

  parameters.put("share", false);
  endpoint_1 = client->workerLoad(worker, &parameters);
  EXPECT_EQ(endpoint_1, "echo-0");

  EXPECT_TRUE(client->modelReady(endpoint));
  EXPECT_TRUE(client->modelReady(endpoint_1));

  client->modelUnload(endpoint);    // unload the first
  client->modelUnload(endpoint);    // this will do nothing
  client->modelUnload(endpoint_1);  // unload first echo-0 worker
  client->modelUnload(endpoint_1);  // unload second echo-0 worker

  while (!client->modelList().empty()) {
    std::this_thread::yield();
  }
}

#ifdef AMDINFER_ENABLE_GRPC
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, workerLoad) { test(client_.get()); }
#endif

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, workerLoad) {
  amdinfer::NativeClient client(&server_);
  test(&client);
}

#ifdef AMDINFER_ENABLE_HTTP
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, workerLoad) { test(client_.get()); }
#endif
