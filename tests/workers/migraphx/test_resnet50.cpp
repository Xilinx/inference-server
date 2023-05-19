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

#include <algorithm>         // for max
#include <array>             // for array
#include <cstdint>           // for int8_t, uint64_t
#include <initializer_list>  // for initializer_list
#include <memory>            // for unique_ptr
#include <opencv2/core.hpp>  // for int8_t
#include <string>            // for string, allocator
#include <vector>            // for vector

#include "amdinfer/amdinfer.hpp"                       // for InferenceRequest
#include "amdinfer/pre_post/image_preprocess.hpp"      // for ImagePreproces...
#include "amdinfer/pre_post/resnet50_postprocess.hpp"  // for resnet50Postpr...
#include "amdinfer/testing/get_path_to_asset.hpp"      // for getPathToAsset
#include "amdinfer/testing/gtest_fixtures.hpp"         // for Message, Suite...

namespace amdinfer {

template <typename T>
using Images = std::vector<std::vector<T>>;

Images<float> preprocessFp32(const std::vector<std::string>& paths) {
  const std::array<float, 3> mean{0.485F, 0.456F, 0.406F};
  const std::array<float, 3> std{4.367F, 4.464F, 4.444F};
  const auto image_size = 224;
  const auto convert_scale = 1 / 255.0;

  amdinfer::pre_post::ImagePreprocessOptions<float, 3> options;
  options.order = amdinfer::pre_post::ImageOrder::NCHW;
  options.height = image_size;
  options.width = image_size;
  options.mean = mean;
  options.std = std;
  options.normalize = true;
  options.convert_color = true;
  options.color_code = cv::COLOR_BGR2RGB;
  options.convert_type = true;
  options.type = CV_32FC3;
  options.convert_scale = convert_scale;
  return amdinfer::pre_post::imagePreprocess(paths, options);
}

Images<fp16> preprocessFp16(const std::vector<std::string>& paths) {
  const std::array<fp16, 3> mean{static_cast<fp16>(0.485F),
                                 static_cast<fp16>(0.456F),
                                 static_cast<fp16>(0.406F)};
  const std::array<fp16, 3> std{static_cast<fp16>(4.367F),
                                static_cast<fp16>(4.464F),
                                static_cast<fp16>(4.444F)};
  const auto image_size = 224;
  const auto convert_scale = 1 / 255.0;

  amdinfer::pre_post::ImagePreprocessOptions<fp16, 3> options;
  options.order = amdinfer::pre_post::ImageOrder::NCHW;
  options.height = image_size;
  options.width = image_size;
  options.mean = mean;
  options.std = std;
  options.normalize = true;
  options.convert_color = true;
  options.color_code = cv::COLOR_BGR2RGB;
  options.convert_type = true;
  options.type = CV_16F;
  options.convert_scale = convert_scale;
  return amdinfer::pre_post::imagePreprocess(paths, options);
}

template <typename T>
std::vector<int> postprocess(const amdinfer::InferenceResponseOutput& output,
                             int k) {
  return amdinfer::pre_post::resnet50Postprocess(
    static_cast<T*>(output.getData()), output.getSize(), k);
}

std::string workerLoad(Client* client, const ParameterMap& parameters) {
  return client->workerLoad("Migraphx", parameters);
}

template <typename T>
std::vector<InferenceRequest> constructRequests(const Images<T>& images) {
  std::vector<InferenceRequest> requests;
  requests.reserve(images.size());

  const std::initializer_list<int64_t> shape = {224, 224, 3};

  DataType type;
  if constexpr (std::is_same_v<T, fp16>) {
    type = DataType::Fp16;
  } else {
    type = DataType::Fp32;
  }

  for (const auto& image : images) {
    requests.emplace_back();
    // NOLINTNEXTLINE(google-readability-casting)
    requests.back().addInputTensor((void*)image.data(), shape, type);
  }

  return requests;
}

template <typename T, int kLength>
void validate(const std::vector<InferenceResponse>& responses,
              const std::array<int, kLength>& golden) {
  const auto k = golden.size();

  for (const auto& response : responses) {
    auto outputs = response.getOutputs();
    ASSERT_EQ(outputs.size(), 1);
    auto top_k = postprocess<T>(outputs[0], k);
    for (auto j = 0U; j < k; ++j) {
      EXPECT_EQ(top_k.at(j), golden.at(j));
    }
  }
}

void validateFp16(const std::vector<InferenceResponse>& responses) {
  const std::array golden{259, 261, 260, 154, 157};
  validate<fp16, golden.size()>(responses, golden);
}

void validateFp32(const std::vector<InferenceResponse>& responses) {
  const std::array golden{259, 261, 157, 260, 154};
  validate<float, golden.size()>(responses, golden);
}

void testFp16(Client* client) {
  if (!serverHasExtension(client, "migraphx")) {
    GTEST_SKIP() << "MIGraphX support required from the server but not found";
  }

  const auto test_asset = getPathToAsset("asset_dog-3619020_640.jpg");
  const auto model = getPathToAsset("onnx_resnet50_fp16");

  amdinfer::ParameterMap parameters;
  parameters.put("model", model);

  auto images = preprocessFp16({test_asset});
  auto endpoint = workerLoad(client, parameters);
  auto requests = constructRequests(images);
  auto responses = inferAsyncOrdered(client, endpoint, requests);
  validateFp16(responses);

  client->workerUnload(endpoint);
  waitUntilModelNotReady(client, endpoint);
}

void testFp32(Client* client) {
  if (!serverHasExtension(client, "migraphx")) {
    GTEST_SKIP() << "MIGraphX support required from the server but not found";
  }

  const auto test_asset = getPathToAsset("asset_dog-3619020_640.jpg");
  const auto model = getPathToAsset("onnx_resnet50");

  amdinfer::ParameterMap parameters;
  parameters.put("model", model);

  auto images = preprocessFp32({test_asset});
  auto endpoint = workerLoad(client, parameters);
  auto requests = constructRequests(images);
  auto responses = inferAsyncOrdered(client, endpoint, requests);
  validateFp32(responses);

  client->workerUnload(endpoint);
  waitUntilModelNotReady(client, endpoint);
}

#ifdef AMDINFER_ENABLE_GRPC
// @pytest.mark.extensions(["migraphx"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, WorkersMigraphxResnet50Fp16) { testFp16(client_.get()); }

// @pytest.mark.extensions(["migraphx"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, WorkersMigraphxResnet50Fp32) { testFp32(client_.get()); }
#endif

// @pytest.mark.extensions(["migraphx"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, WorkersMigraphxResnet50Fp16) {
  NativeClient client(&server_);
  testFp16(&client);
}

// @pytest.mark.extensions(["migraphx"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, WorkersMigraphxResnet50Fp32) {
  NativeClient client(&server_);
  testFp32(&client);
}

#ifdef AMDINFER_ENABLE_HTTP
// @pytest.mark.extensions(["migraphx"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, WorkersMigraphxResnet50Fp16) { testFp16(client_.get()); }

// @pytest.mark.extensions(["migraphx"])
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, WorkersMigraphxResnet50Fp32) { testFp32(client_.get()); }
#endif

}  // namespace amdinfer
