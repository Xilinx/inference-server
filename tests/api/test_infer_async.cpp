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

#include <cstdint>  // for uint32_t, uint64_t
#include <vector>   // for vector, allocator

#include "amdinfer/clients/native.hpp"           // for NativeClient, inferAs...
#include "amdinfer/core/data_types.hpp"          // for DataType, DataType::U...
#include "amdinfer/core/inference_request.hpp"   // for InferenceRequest
#include "amdinfer/core/inference_response.hpp"  // for InferenceResponse
#include "amdinfer/testing/gtest_fixtures.hpp"   // for AssertionResult, Message

namespace amdinfer {

#ifdef AMDINFER_ENABLE_HTTP
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, Ordered) {
  NativeClient client(&server_);
  auto endpoint =
    client.workerLoad("cplusplus", {{"model"}, {std::string{"echo"}}});
  EXPECT_EQ(endpoint, "cplusplus");

  std::vector<uint32_t> img_data;
  const auto shape = {1L};
  const auto size = 1;
  img_data.reserve(size);
  img_data.push_back(1);

  amdinfer::InferenceRequest request;
  request.addInputTensor(img_data.data(), shape, amdinfer::DataType::Uint32);

  const auto data_size = 10;
  std::vector<InferenceRequest> reqs;
  reqs.reserve(data_size);
  for (auto i = 0; i < data_size; ++i) {
    reqs.push_back(request);
  }

  auto resps = inferAsyncOrdered(&client, endpoint, reqs);
  EXPECT_EQ(resps.size(), data_size);
  for (const auto& resp : resps) {
    EXPECT_FALSE(resp.isError());
  }

  client.workerUnload(endpoint);
}
#endif

}  // namespace amdinfer
