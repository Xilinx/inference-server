// Copyright 2022 Advanced Micro Devices Inc.
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

/**
 * @file
 * @brief Performance testing for the native client
 */

#include <chrono>
#include <filesystem>
#include <queue>
#include <string>
#include <vector>

#include "proteus/client_operators/infer_async.hpp"
#include "proteus/proteus.hpp"                 // for InferenceResponse, Grp...
#include "proteus/testing/gtest_fixtures.hpp"  // for GrpcFixture
#include "proteus/util/ctpl.h"                 // for thread_pool
#include "proteus/util/pre_post/image_preprocess.hpp"

namespace fs = std::filesystem;

void test(proteus::Client* client) {
  const fs::path kRoot{std::getenv("PROTEUS_ROOT")};

  // for this resnet50 model, use the following values
  const std::string worker{"ptzendnn"};
  const auto kGraph = kRoot / "external/pytorch_models/resnet50_pretrained.pt";
  const auto kImageLocation = kRoot / "tests/assets/dog-3619020_640.jpg";
  const auto kInputSize = 224;
  const auto kOutputClasses = 1000;
  const auto kBatchSize = 20;

  const auto kWarmupRequests = 4;
  const auto kRequests = 40;

  proteus::RequestParameters parameters;
  parameters.put("model", kGraph.string());
  parameters.put("input_size", kInputSize);
  parameters.put("output_classes", kOutputClasses);
  parameters.put("batch_size", kBatchSize);

  auto endpoint = client->workerLoad(worker, &parameters);
  EXPECT_EQ(endpoint, worker);

  std::vector<std::string> paths{kImageLocation};
  proteus::util::ImagePreprocessOptions<float, 3> options_;
  options_.normalize = true;
  options_.order = proteus::util::ImageOrder::NCHW;
  options_.mean = {0.485, 0.456, 0.406};
  options_.std = {4.367, 4.464, 4.444};
  options_.convert_color = true;
  options_.color_code = cv::COLOR_BGR2RGB;
  options_.convert_type = true;
  options_.type = CV_32FC3;
  options_.convert_scale = 1.0 / 255.0;
  auto images = proteus::util::imagePreprocess(paths, options_);

  proteus::InferenceRequest request;
  request.addInputTensor(images[0].data(), {3, kInputSize, kInputSize},
                         proteus::DataType::FLOAT32);

  // warm up
  for (auto i = 0; i < kWarmupRequests; ++i) {
    client->modelInfer(worker, request);
  }

  auto start = std::chrono::high_resolution_clock::now();
  for (auto i = 0; i < kRequests; ++i) {
    auto response = client->modelInfer(worker, request);
    EXPECT_FALSE(response.isError());
  }
  auto stop = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> duration = stop - start;
  std::cout << "-----\n";
  std::cout << "sync average time taken for images: " << duration.count()
            << "ms" << std::endl;
  std::cout << "-----\n";

  std::vector<proteus::InferenceRequest> requests;
  requests.reserve(kRequests);
  for (auto i = 0; i < kRequests; ++i) {
    requests.push_back(request);
  }
  start = std::chrono::high_resolution_clock::now();
  auto responses = proteus::inferAsyncOrdered(client, worker, requests);
  stop = std::chrono::high_resolution_clock::now();
  duration = stop - start;
  std::cout << "-----\n";
  std::cout << "async average time taken for images: " << duration.count()
            << "ms" << std::endl;
  std::cout << "-----\n";
}

#ifdef PROTEUS_ENABLE_GRPC
// @pytest.mark.perf(group="clients")
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, ModelInfer) { test(client_.get()); }
#endif

// @pytest.mark.perf(group="clients")
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, ModelInfer) {
  proteus::NativeClient client;
  test(&client);
}

#ifdef PROTEUS_ENABLE_HTTP
// @pytest.mark.perf(group="clients")
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, ModelInfer) { test(client_.get()); }
#endif
