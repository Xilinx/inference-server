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

#include "fixture.hpp"

TEST_F(Grpc, model_infer) {
  auto endpoint = client_->modelLoad("echo");
  EXPECT_EQ(endpoint, "echo");

  std::vector<uint8_t> imgData;
  auto shape = {1UL};
  auto size = 1 * sizeof(uint8_t);
  imgData.reserve(size);
  imgData.push_back(1);

  proteus::InferenceRequest request;
  request.addInputTensor(static_cast<void*>(imgData.data()), shape,
                         proteus::types::DataType::UINT8);

  auto response = client_->modelInfer(endpoint, request);

  EXPECT_FALSE(response.isError());
  EXPECT_EQ(response.getID(), "");
  EXPECT_EQ(response.getModel(), "echo");

  auto outputs = response.getOutputs();
  EXPECT_EQ(outputs.size(), 1);
  for (auto& output : outputs) {
    auto* data = static_cast<std::vector<uint32_t>*>(output.getData());
    EXPECT_EQ((*data)[0], 2);
  }
}
