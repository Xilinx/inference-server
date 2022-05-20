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

#include <cstdint>    // for uint32_t
#include <future>     // for future
#include <memory>     // for allocator, make_shared, __shared_ptr_...
#include <stdexcept>  // for runtime_error
#include <tuple>      // for tuple, get
#include <vector>     // for vector

#include "gtest/gtest.h"        // for Message, AssertionResult, TestPartResult
#include "proteus/proteus.hpp"  // for InferenceRequestInput, RequestParameters

namespace proteus {

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

    InferenceRequestInput input_1;
    input_1.setName("echo");
    input_1.setDatatype(DataType::UINT32);
    input_1.setShape({1});
    input_1.setData((void*)(&(inputs_[1])));
    if (add_input_parameters) {
      auto parameters = std::make_shared<RequestParameters>();
      parameters->put("key_1", 1);
      input_1.setParameters(parameters);
    }

    InferenceRequest request;
    for (auto i = 0; i < multiplier; i++) {
      request.addInputTensor(input_0);
      request.addInputTensor(input_1);
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
    EXPECT_EQ(outputs.size(), multiplier * 2);

    for (auto i = 0; i < multiplier; i++) {
      for (auto j = 0; j < 2; j++) {
        auto output = outputs[(i * 2 + j)];
        auto* data = static_cast<std::vector<uint32_t>*>(output.getData());
        EXPECT_EQ(data->size(), 1);
        EXPECT_EQ((*data)[0], golden_outputs_[j]);
        EXPECT_EQ(output.getDatatype(), DataType::UINT32);
        EXPECT_EQ(output.getName(), "echo");
        EXPECT_TRUE(output.getParameters()->empty());
        auto shape = output.getShape();
        EXPECT_EQ(shape.size(), 1);
        EXPECT_EQ(shape[0], 1);
      }
    }

    if (add_id) {
      EXPECT_EQ(response.getID(), "hello_world");
    }
  }

  static void SetUpTestSuite() { proteus::initialize(); };

  static void TearDownTestSuite() { proteus::terminate(); }

 private:
  const int inputs_[2] = {3, 5};
  const int golden_outputs_[2] = {4, 6};
};

TEST_P(EchoParamFixture, EchoNative) {
  const auto params = GetParam();
  auto multiplier = std::get<4>(params);

  proteus::NativeClient client;
  client.modelLoad("echo", nullptr);

  auto request = this->construct_request();

  auto response = client.modelInfer("echo", request);

  if (multiplier == 30) {
    EXPECT_TRUE(response.isError());
    EXPECT_EQ(response.getError(), "Too many input tensors for this model");
  } else {
    validate(response);
  }

  client.modelUnload("echo");
}

TEST_P(EchoParamFixture, EchoGrpc) {
  const auto params = GetParam();
  auto multiplier = std::get<4>(params);

  proteus::startGrpcServer(50051);
  auto client = proteus::GrpcClient("localhost:50051");

  client.modelLoad("echo", nullptr);

  auto request = this->construct_request();

  if (multiplier == 30) {
    try {
      auto response = client.modelInfer("echo", request);
      FAIL() << "No runtime error thrown";
    } catch (const std::runtime_error& e) {
      EXPECT_STREQ(e.what(), "Too many input tensors for this model");
    }
  } else {
    auto response = client.modelInfer("echo", request);
    validate(response);
  }

  client.modelUnload("echo");
  proteus::stopGrpcServer();
}

// add_id, add_input_parameters, add_request_parameters, add_outputs, multiplier
std::tuple<bool, bool, bool, bool, int> configs[] = {
  {true, true, true, true, 1},      {true, true, false, true, 1},
  {true, false, false, true, 1},    {false, false, false, true, 1},
  {false, false, false, false, 1},  {false, false, false, false, 10},
  {false, false, false, false, 30},
};
INSTANTIATE_TEST_SUITE_P(Echo, EchoParamFixture, testing::ValuesIn(configs));

}  // namespace proteus
