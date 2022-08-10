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

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "gtest/gtest.h"        // for Message, AssertionResult, TestPartResult
#include "proteus/proteus.hpp"  // for InferenceRequestInput, RequestParameters
#include "proteus/testing/gtest_fixtures.hpp"  // for AssertionResult, Suite...

namespace proteus {

void test(proteus::Client* client) {
  const std::string model = "mnist";

  EXPECT_TRUE(client->modelList().empty());

  client->modelLoad(model, nullptr);
  EXPECT_TRUE(client->modelReady(model));

  std::string path =
    std::string(std::getenv("PROTEUS_ROOT")) + "/tests/assets/nine_9723.jpg";
  auto img = cv::imread(path);
  cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
  img.convertTo(img, CV_32FC3, 1.0 / 255.0);

  InferenceRequestInput input_0;
  input_0.setName("input0");
  input_0.setDatatype(DataType::FP32);
  input_0.setShape({28, 28, 1});
  input_0.setData((void*)(img.data));

  InferenceRequest request;
  request.addInputTensor(input_0);

  auto response = client->modelInfer(model, request);

  const auto& outputs = response.getOutputs();
  EXPECT_EQ(outputs.size(), 1);
  const auto& output = outputs[0];
  EXPECT_EQ(output.getSize(), 10);
  EXPECT_EQ(output.getDatatype(), DataType::FP32);
  const auto data = *static_cast<std::vector<float>*>(output.getData());
  EXPECT_EQ(data.size(), 10);

  float max = 0;
  int index = -1;
  for (auto i = 0; i < 10; i++) {
    if (data[i] > max) {
      max = data[i];
      index = i;
    }
  }
  EXPECT_EQ(index, 9);

  client->modelUnload(model);  // unload the model

  while (!client->modelList().empty()) {
    std::this_thread::yield();
  }
}

#ifdef PROTEUS_ENABLE_GRPC
// @pytest.mark.extensions(["tfzendnn"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, mnist) { test(client_.get()); }
#endif

// @pytest.mark.extensions(["tfzendnn"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, mnist) {
  NativeClient client;
  test(&client);
}

#ifdef PROTEUS_ENABLE_HTTP
// @pytest.mark.extensions(["tfzendnn"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, mnist) { test(client_.get()); }
#endif

}  // namespace proteus
