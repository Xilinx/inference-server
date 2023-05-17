// Copyright 2023 Advanced Micro Devices, Inc.
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
#include "amdinfer/util/base64.hpp"
#include "amdinfer/util/containers.hpp"
#include "amdinfer/util/filesystem.hpp"  // for readFile

namespace amdinfer {

void test(const amdinfer::Client* client, const std::string& version) {
  const auto path = getPathToAsset("asset_dog-3619020_640.jpg");

  const std::string model = "invert_image";
  const std::vector<std::string> endpoints{"invert_image", "execute",
                                           "invert_image_postprocess"};

  EXPECT_TRUE(client->modelList().empty());

  client->modelLoad(model, {}, version);
  for (const auto& endpoint : endpoints) {
    EXPECT_TRUE(client->modelReady(endpoint, version));
  }

  auto img = cv::imread(path);
  auto golden_img = img;
  cv::bitwise_not(img, golden_img);
  auto golden_shape = golden_img.rows * golden_img.cols * 3;

  // read the image as a jpeg
  auto raw_data = util::readFile(path);
  auto encoded_data = util::base64Encode(std::move(raw_data));

  InferenceRequestInput input_0;
  input_0.setName("input0");
  input_0.setDatatype(DataType::Bytes);
  input_0.setShape({encoded_data.size()});
  input_0.setData(encoded_data.data());

  InferenceRequest request;
  request.addInputTensor(input_0);

  auto response = client->modelInfer(endpoints[0], request, version);

  const auto& outputs = response.getOutputs();
  EXPECT_EQ(outputs.size(), 1);
  const auto& output = outputs[0];
  const auto& output_shape = output.getShape();
  EXPECT_TRUE(output_shape.size() == 1);
  EXPECT_EQ(output.getDatatype(), DataType::Bytes);
  const auto* data = static_cast<char*>(output.getData());

  auto decoded_data = util::base64Decode(data, output_shape.at(0));
  std::vector<char> buffer(decoded_data.begin(), decoded_data.end());
  auto received_image = cv::imdecode(buffer, cv::IMREAD_UNCHANGED);
  ASSERT_FALSE(received_image.empty());

  cv::Mat diff;
  cv::absdiff(received_image, golden_img, diff);
  auto manhattan_norm = cv::sum(cv::sum(diff))[0];
  auto manhattan_norm_per_pixel = manhattan_norm / golden_shape;
  EXPECT_TRUE(manhattan_norm_per_pixel < 2);

  unloadModels(client, endpoints, version);

  while (!client->modelList().empty()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

#ifdef AMDINFER_ENABLE_GRPC
// @pytest.mark.extensions(["tfzendnn"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, InvertImage) { test(client_.get(), ""); }

// @pytest.mark.extensions(["tfzendnn"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, InvertImageVersioned) { test(client_.get(), "1"); }
#endif

// @pytest.mark.extensions(["tfzendnn"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, InvertImage) {
  NativeClient client(&server_);
  test(&client, "");
}

// @pytest.mark.extensions(["tfzendnn"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, InvertImageVersioned) {
  NativeClient client(&server_);
  test(&client, "1");
}

#ifdef AMDINFER_ENABLE_HTTP
// @pytest.mark.extensions(["tfzendnn"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, InvertImage) { test(client_.get(), ""); }

// @pytest.mark.extensions(["tfzendnn"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, InvertImageVersioned) { test(client_.get(), "1"); }
#endif

}  // namespace amdinfer
