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

#include <array>    // for array
#include <chrono>   // for seconds
#include <cstdint>  // for uint32_t
#include <memory>   // for allocator, make_shared, __shared_ptr_...
#include <string>   // for string
#include <thread>   // for sleep_for
#include <vector>   // for vector

#include "amdinfer/amdinfer.hpp"  // for ParameterMap, InferenceRequestInput
#include "gtest/gtest.h"  // for ParamIteratorInterface, Message, Test...

namespace amdinfer {

struct Params {
  bool add_id;
  bool add_input_parameters;
  bool add_request_parameters;
  bool add_outputs;
  int multiplier;
};

class EchoParamFixture : public testing::TestWithParam<Params> {
 public:
  const std::array<int, 1> inputs{3};
  const std::array<int, 1> golden_outputs{4};

  [[nodiscard]] InferenceRequest constructRequest() const {
    const auto params = GetParam();

    InferenceRequestInput input_0;
    input_0.setName("echo");
    input_0.setDatatype(DataType::Uint32);
    input_0.setShape({1});
    // NOLINTNEXTLINE(google-readability-casting)
    input_0.setData((void*)(&(inputs[0])));
    if (params.add_input_parameters) {
      ParameterMap parameters;
      parameters.put("key_0", "value");
      input_0.setParameters(parameters);
    }

    InferenceRequest request;
    for (auto i = 0; i < params.multiplier; i++) {
      request.addInputTensor(input_0);
    }

    if (params.add_outputs) {
      InferenceRequestOutput output;
      output.setName("echo");
      if (params.add_input_parameters) {
        ParameterMap parameters;
        parameters.put("key", "another_value");
        output.setParameters(parameters);
      }
      for (auto i = 0; i < params.multiplier; i++) {
        request.addOutputTensor(output);
      }
    }

    if (params.add_id) {
      request.setID("hello_world");
    }

    if (params.add_request_parameters) {
      const auto key_3 = 1.2;  // arbitrary value
      ParameterMap parameters;
      parameters.put("key_2", true);
      parameters.put("key_3", key_3);
      request.setParameters(parameters);
    }

    return request;
  }

  void validate(InferenceResponse response) {
    const auto params = GetParam();
    auto add_id = params.add_id;
    auto multiplier = params.multiplier;

    ASSERT_FALSE(response.isError());
    EXPECT_EQ(response.getModel(), "echo");
    auto outputs = response.getOutputs();
    EXPECT_EQ(outputs.size(), multiplier);

    for (auto i = 0; i < multiplier; i++) {
      const auto& output = outputs[i];
      const auto* data = static_cast<uint32_t*>(output.getData());
      EXPECT_EQ(data[0], golden_outputs[0]);
      EXPECT_EQ(output.getDatatype(), DataType::Uint32);
      EXPECT_EQ(output.getName(), "echo");
      EXPECT_TRUE(output.getParameters().empty());
      auto shape = output.getShape();
      EXPECT_EQ(shape.size(), 1);
      EXPECT_EQ(shape[0], 1);
    }

    if (add_id) {
      EXPECT_EQ(response.getID(), "hello_world");
    }
  }

  amdinfer::Server server;
};

TEST_P(EchoParamFixture, EchoNative) {  // NOLINT
  amdinfer::NativeClient client(&server);
  const auto endpoint = client.workerLoad("echo", {});

  auto request = this->constructRequest();

  auto response = client.modelInfer(endpoint, request);

  validate(response);

  client.modelUnload(endpoint);
}

#ifdef AMDINFER_ENABLE_GRPC
TEST_P(EchoParamFixture, EchoGrpc) {  // NOLINT
  server.startGrpc(kDefaultGrpcPort);
  auto client =
    amdinfer::GrpcClient("localhost:" + std::to_string(kDefaultGrpcPort));
  while (!client.serverLive()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  const auto endpoint = client.workerLoad("echo", {});

  auto request = this->constructRequest();

  auto response = client.modelInfer(endpoint, request);
  validate(response);

  client.modelUnload(endpoint);
}
#endif

// add_id, add_input_parameters, add_request_parameters, add_outputs, multiplier
const std::array kConfigs{
  Params{true, true, true, true, 1},     Params{true, true, false, true, 1},
  Params{true, false, false, true, 1},   Params{false, false, false, true, 1},
  Params{false, false, false, false, 1},
};

// NOLINTNEXTLINE(cert-err58-cpp)
INSTANTIATE_TEST_SUITE_P(Echo, EchoParamFixture, testing::ValuesIn(kConfigs));

}  // namespace amdinfer
