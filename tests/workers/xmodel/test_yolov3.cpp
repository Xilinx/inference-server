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

#include <algorithm>            // for max
#include <array>                // for array
#include <cstdint>              // for uint64_t
#include <initializer_list>     // for initializer_list
#include <memory>               // for unique_ptr, alloca...
#include <opencv2/core.hpp>     // for CV_32FC3
#include <opencv2/imgproc.hpp>  // for COLOR_BGR2RGB
#include <string>               // for string, allocator
#include <vector>               // for vector

#include "amdinfer/amdinfer.hpp"                   // for InferenceRequest
#include "amdinfer/pre_post/image_preprocess.hpp"  // for ImagePreprocessOpt...
#include "amdinfer/testing/get_path_to_asset.hpp"  // for getPathToAsset
#include "amdinfer/testing/gtest_fixtures.hpp"     // for SuiteApiResolver

namespace amdinfer {

using Images = std::vector<std::vector<float>>;

Images preprocess(const std::vector<std::string>& paths) {
  // this model uses images of 416x416
  const int image_size = 416;
  const auto convert_scale = 1 / 255.0;

  amdinfer::pre_post::ImagePreprocessOptions<float, 3> options;
  options.height = image_size;
  options.width = image_size;

  options.resize_algorithm = pre_post::ResizeAlgorithm::CenterCrop;

  options.convert_color = true;
  options.color_code = cv::COLOR_BGR2RGB;

  options.convert_type = true;
  options.type = CV_32FC3;
  options.convert_scale = convert_scale;

  options.normalize = true;
  options.order = amdinfer::pre_post::ImageOrder::NHWC;
  options.mean = {0, 0, 0};
  options.std = {1, 1, 1};
  return pre_post::imagePreprocess(paths, options);
}

void postprocess(const amdinfer::InferenceResponseOutput& output) {
  // TODO(varunsh): needs to be implemented
  (void)output;
}

std::string workerLoad(Client* client, RequestParameters* parameters) {
  return client->workerLoad("Xmodel", parameters);
}

std::vector<InferenceRequest> constructRequests(const Images& images) {
  std::vector<InferenceRequest> requests;
  requests.reserve(images.size());

  const std::initializer_list<uint64_t> shape = {224, 224, 3};

  for (const auto& image : images) {
    requests.emplace_back();
    // NOLINTNEXTLINE(google-readability-casting)
    requests.back().addInputTensor((void*)image.data(), shape, DataType::Int8);
  }

  return requests;
}

void validate(const std::vector<InferenceResponse>& responses) {
  // TODO(varunsh): add post-processing and verify output
  for (const auto& response : responses) {
    auto outputs = response.getOutputs();
    EXPECT_EQ(outputs.size(), 3);
  }
}

void test0(Client* client) {
  if (!serverHasExtension(client, "vitis")) {
    GTEST_SKIP() << "Vitis AI support required from the server but not found";
  }

  if (!client->hasHardware("DPUCADF8H", 1)) {
    GTEST_SKIP() << "At least one DPUCADF8H required on the server";
  }

  const auto test_asset = getPathToAsset("asset_bicycle-384566_640.jpg");
  const auto xmodel = getPathToAsset("u250_yolov3");

  amdinfer::RequestParameters parameters;
  parameters.put("model", xmodel);

  auto images = preprocess({test_asset});
  auto endpoint = workerLoad(client, &parameters);
  auto requests = constructRequests(images);
  auto responses = inferAsyncOrdered(client, endpoint, requests);
  validate(responses);
}

#ifdef AMDINFER_ENABLE_GRPC
// @pytest.mark.extensions(["vitis"])
// @pytest.mark.fpgas("DPUCADF8H", 1)
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, WorkersXmodelYolov3) { test0(client_.get()); }
#endif

// @pytest.mark.extensions(["vitis"])
// @pytest.mark.fpgas("DPUCADF8H", 1)
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, WorkersXmodelYolov3) {
  NativeClient client;
  test0(&client);
}

#ifdef AMDINFER_ENABLE_HTTP
// @pytest.mark.extensions(["vitis"])
// @pytest.mark.fpgas("DPUCADF8H", 1)
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, WorkersXmodelYolov3) { test0(client_.get()); }
#endif

}  // namespace amdinfer
