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

#include <cstdlib>                // for getenv
#include <memory>                 // for allocator, unique_ptr
#include <opencv2/core.hpp>       // for Mat, CV_32FC3
#include <opencv2/imgcodecs.hpp>  // for imread
#include <opencv2/imgproc.hpp>    // for cvtColor, COLOR_BGR2GRAY
#include <string>                 // for string, operator+
#include <thread>                 // for yield
#include <vector>                 // for vector

#include "amdinfer/amdinfer.hpp"                   // for InferenceRequestInput
#include "amdinfer/testing/get_path_to_asset.hpp"  // for getPathToAsset
#include "amdinfer/testing/gtest_fixtures.hpp"  // for AssertionResult, Message

namespace amdinfer {

void test(amdinfer::Client* client) {
  if (!serverHasExtension(client, "tfzendnn")) {
    GTEST_SKIP() << "This test requires TF+ZenDNN support.";
  }

  const auto path = getPathToAsset("asset_nine_9723.jpg");
  getPathToAsset("tf_mnist");

  const std::string model = "mnist";

  EXPECT_TRUE(client->modelList().empty());

  client->modelLoad(model, nullptr);
  EXPECT_TRUE(client->modelReady(model));

  auto img = cv::imread(path);
  cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
  img.convertTo(img, CV_32FC3, 1.0 / 255.0);

  InferenceRequestInput input_0;
  input_0.setName("input0");
  input_0.setDatatype(DataType::FP32);
  input_0.setShape({28, 28, 1});
  input_0.setData(img.data);

  InferenceRequest request;
  request.addInputTensor(input_0);

  auto response = client->modelInfer(model, request);

  const auto& outputs = response.getOutputs();
  EXPECT_EQ(outputs.size(), 1);
  const auto& output = outputs[0];
  EXPECT_EQ(output.getSize(), 10);
  EXPECT_EQ(output.getDatatype(), DataType::FP32);
  const auto* data = static_cast<float*>(output.getData());

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

}  // namespace amdinfer
