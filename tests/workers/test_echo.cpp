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

#include <chrono>   // for seconds
#include <cstdint>  // for uint32_t
#include <memory>   // for allocator, make_shared, __shared_ptr_...
#include <thread>   // for sleep_for
#include <tuple>    // for tuple, get
#include <vector>   // for vector

#include "amdinfer/amdinfer.hpp"  // for RequestParameters, InferenceRequestInput
#include "gtest/gtest.h"  // for ParamIteratorInterface, Message, Test...

namespace amdinfer {

class EchoParamFixture
  : public testing::TestWithParam<std::tuple<bool, bool, bool, bool, int>> {
 public:
  InferenceRequest construct_request() {
    const auto [add_id, add_input_parameters, add_request_parameters,
                add_outputs, multiplier] = GetParam();

    InferenceRequestInput input_0;
    input_0.setName("echo");
    input_0.setDatatype(DataType::UINT32);
    input_0.setShape({1});
    input_0.setData((void*)(&(inputs_[0])));
    if (add_input_parameters) {
      auto parameters = std::make_shared<RequestParameters>();
      parameters->put("key_0", "value");
      input_0.setParameters(parameters);
    }

    InferenceRequest request;
    for (auto i = 0; i < multiplier; i++) {
      request.addInputTensor(input_0);
    }

    if (add_outputs) {
      InferenceRequestOutput output;
      output.setName("echo");
      if (add_input_parameters) {
        auto parameters = std::make_shared<RequestParameters>();
        parameters->put("key", "another_value");
        output.setParameters(parameters);
      }
      for (auto i = 0; i < multiplier; i++) {
        request.addOutputTensor(output);
      }
    }

    if (add_id) {
      request.setID("hello_world");
    }

    if (add_request_parameters) {
      auto parameters = std::make_shared<RequestParameters>();
      parameters->put("key_2", true);
      parameters->put("key_3", 1.2);
      request.setParameters(parameters);
    }

    return request;
  }

  void validate(InferenceResponse response) {
    const auto params = GetParam();
    auto add_id = std::get<0>(params);
    auto multiplier = std::get<4>(params);

    EXPECT_FALSE(response.isError());
    EXPECT_EQ(response.getModel(), "echo");
    auto outputs = response.getOutputs();
    EXPECT_EQ(outputs.size(), multiplier);

    for (auto i = 0; i < multiplier; i++) {
      const auto& output = outputs[i];
      const auto* data = static_cast<uint32_t*>(output.getData());
      EXPECT_EQ(data[0], golden_outputs_[0]);
      EXPECT_EQ(output.getDatatype(), DataType::UINT32);
      EXPECT_EQ(output.getName(), "echo");
      EXPECT_TRUE(output.getParameters()->empty());
      auto shape = output.getShape();
      EXPECT_EQ(shape.size(), 1);
      EXPECT_EQ(shape[0], 1);
    }

    if (add_id) {
      EXPECT_EQ(response.getID(), "hello_world");
    }
  }

  amdinfer::Server server_;

 private:
  const int inputs_[1] = {3};
  const int golden_outputs_[1] = {4};
};

TEST_P(EchoParamFixture, EchoNative) {
  amdinfer::NativeClient client;
  client.workerLoad("echo", nullptr);

  auto request = this->construct_request();

  auto response = client.modelInfer("echo", request);

  validate(response);

  client.modelUnload("echo");
}

#ifdef AMDINFER_ENABLE_GRPC
TEST_P(EchoParamFixture, EchoGrpc) {
  server_.startGrpc(50051);
  auto client = amdinfer::GrpcClient("localhost:50051");
  while (!client.serverLive()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  client.workerLoad("echo", nullptr);

  auto request = this->construct_request();

  auto response = client.modelInfer("echo", request);
  validate(response);

  client.modelUnload("echo");
}
#endif

// add_id, add_input_parameters, add_request_parameters, add_outputs, multiplier
const std::tuple<bool, bool, bool, bool, int> configs[] = {
  {true, true, true, true, 1},     {true, true, false, true, 1},
  {true, false, false, true, 1},   {false, false, false, true, 1},
  {false, false, false, false, 1},
};
INSTANTIATE_TEST_SUITE_P(Echo, EchoParamFixture, testing::ValuesIn(configs));

}  // namespace amdinfer
