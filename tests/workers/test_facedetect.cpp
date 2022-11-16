// Copyright 2021 Xilinx, Inc.
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

#include <cstdlib>  // for getenv, abs

#include "amdinfer/testing/get_path_to_asset.hpp"  // for getPathToAsset
#include "facedetect.hpp"                          // IWYU pragma: associated
#include "gtest/gtest.h"  // for Test, AssertionResult, EXPECT_EQ

const int gold_response_size = 6;
const float gold_response_output[gold_response_size] = {
  -1, 0.9937100410461426, 268, 78.728, 158, 170.800,
};

std::string prepareDirectory() {
  fs::path temp_dir =
    fs::temp_directory_path() / "amdinfer/tests/cpp/native/facedetect";
  fs::create_directories(temp_dir);
  const auto src_file =
    fs::path(amdinfer::getPathToAsset("asset_girl-1867092_640.jpg"));
  fs::copy_file(src_file, temp_dir / src_file.filename(),
                fs::copy_options::skip_existing);
  return temp_dir;
}

void dequeue_validate(FutureQueue& my_queue, int num_images) {
  std::future<amdinfer::InferenceResponse> element;
  for (int i = 0; i < num_images; i++) {
    my_queue.wait_dequeue(element);
    auto results = element.get();

    EXPECT_STREQ(results.getID().c_str(), "");
    EXPECT_STREQ(results.getModel().c_str(), "facedetect");
    auto outputs = results.getOutputs();
    EXPECT_EQ(outputs.size(), 1);
    for (auto& output : outputs) {
      const auto* data = static_cast<float*>(output.getData());
      auto size = output.getSize();
      EXPECT_STREQ(output.getName().c_str(), "");
      EXPECT_STREQ(output.getDatatype().str(), "FP32");
      auto* parameters = output.getParameters();
      ASSERT_NE(parameters, nullptr);
      EXPECT_TRUE(parameters->empty());
      auto num_boxes = gold_response_size / 6;
      auto shape = output.getShape();
      EXPECT_EQ(shape.size(), 2);
      EXPECT_EQ(shape[0], 6);
      EXPECT_EQ(shape[1], num_boxes);
      EXPECT_EQ(size, gold_response_size);
      for (size_t i = 0; i < gold_response_size; i++) {
        // expect that the response values are within 1% of the golden
        const float abs_error = std::abs(gold_response_output[i] * 0.05);
        EXPECT_NEAR(data[i], gold_response_output[i], abs_error);
      }
    }
  }
}

// @pytest.mark.extensions(["vitis"])
// @pytest.mark.fpgas("DPUCADF8H", 1)
TEST(Native, Facedetect) {
  amdinfer::Server server;
  amdinfer::NativeClient client;

  auto fpgas_exist = client.hasHardware("DPUCADF8H", 1);
  if (!fpgas_exist) {
    GTEST_SKIP() << "No FPGAs available";
  }

  auto path = prepareDirectory();
  auto worker_name = load(1);
  auto image_paths = getImages(path);
  auto num_images = image_paths.size();

  FutureQueue my_queue;
  run(image_paths, 1, worker_name, my_queue);

  dequeue_validate(my_queue, num_images);
}
