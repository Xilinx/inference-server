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

#include <cstdint>  // for uint8_t, uint64_t, uin...
#include <memory>   // for allocator, unique_ptr
#include <vector>   // for vector

#include "amdinfer/amdinfer.hpp"                // for InferenceResponse, Grp...
#include "amdinfer/testing/gtest_fixtures.hpp"  // for GrpcFixture

namespace amdinfer {

void test(const Client* client) {
  Chain chain{{"cplusplus"}, {{{"model"}, {"echo"}}}};
  chain.load(client);
  const auto& endpoint = chain.get();
  EXPECT_EQ(endpoint, "cplusplus");

  std::vector<uint32_t> img_data;
  auto shape = {1UL};
  auto size = 1;
  img_data.reserve(size);
  img_data.push_back(1);

  InferenceRequest request;
  request.addInputTensor(static_cast<void*>(img_data.data()), shape,
                         DataType::Uint32);

  auto response = client->modelInfer(endpoint, request);

  EXPECT_FALSE(response.isError());
  EXPECT_EQ(response.getID(), "");
  EXPECT_EQ(response.getModel(), "Responder");

  auto outputs = response.getOutputs();
  EXPECT_EQ(outputs.size(), 1);
  for (const auto& output : outputs) {
    const auto* data = static_cast<uint32_t*>(output.getData());
    EXPECT_EQ(data[0], 2);
  }

  chain.unload(client);
}

#ifdef AMDINFER_ENABLE_GRPC
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, ModelInfer) { test(client_.get()); }
#endif

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, ModelInfer) {
  NativeClient client(&server_);
  test(&client);
}

#ifdef AMDINFER_ENABLE_HTTP
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, ModelInfer) { test(client_.get()); }
#endif

}  // namespace amdinfer
