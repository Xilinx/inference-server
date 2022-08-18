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

#include <cstdint>  // for uint8_t, uint64_t, uin...
#include <memory>   // for allocator, unique_ptr
#include <queue>    // for queue
#include <vector>   // for vector

#include "proteus/proteus.hpp"                 // for InferenceResponse, Grp...
#include "proteus/testing/gtest_fixtures.hpp"  // for GrpcFixture

void test(proteus::Client* client) {
  auto endpoint = client->workerLoad("echo", nullptr);
  EXPECT_EQ(endpoint, "echo");

  std::vector<uint32_t> imgData;
  auto shape = {1UL};
  auto size = 1;
  imgData.reserve(size);
  imgData.push_back(1);

  proteus::InferenceRequest request;
  request.addInputTensor(static_cast<void*>(imgData.data()), shape,
                         proteus::DataType::UINT32);

  const auto num_requests = 16;
  std::queue<proteus::InferenceResponseFuture> q;
  for (auto i = 0; i < num_requests; ++i) {
    q.push(client->modelInferAsync(endpoint, request));
  }

  for (auto i = 0; i < num_requests; ++i) {
    auto& future = q.front();
    auto response = future.get();
    q.pop();

    EXPECT_FALSE(response.isError());
    EXPECT_EQ(response.getID(), "");
    EXPECT_EQ(response.getModel(), "echo");

    auto outputs = response.getOutputs();
    EXPECT_EQ(outputs.size(), 1);
    for (const auto& output : outputs) {
      auto* data = static_cast<std::vector<uint32_t>*>(output.getData());
      EXPECT_EQ(data->size(), 1);
      EXPECT_EQ((*data)[0], 2);
    }
  }

  client->modelUnload(endpoint);
}

#ifdef PROTEUS_ENABLE_GRPC
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, ModelInfer) { test(client_.get()); }
#endif

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, ModelInfer) {
  proteus::NativeClient client;
  test(&client);
}

#ifdef PROTEUS_ENABLE_HTTP
TEST_F(HttpFixture, ModelInfer) { test(client_.get()); }
#endif
