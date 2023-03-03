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

using Images = std::vector<std::vector<int8_t>>;

Images preprocess(const std::vector<std::string>& paths) {
  const std::array<int8_t, 3> mean{123, 107, 104};
  const std::array<int8_t, 3> std{1, 1, 1};

  amdinfer::pre_post::ImagePreprocessOptions<int8_t, 3> options;
  options.order = amdinfer::pre_post::ImageOrder::NHWC;
  options.mean = mean;
  options.std = std;
  options.normalize = true;
  return pre_post::imagePreprocess(paths, options);
}

std::vector<int> postprocess(const amdinfer::InferenceResponseOutput& output,
                             int k) {
  return amdinfer::pre_post::resnet50Postprocess(
    static_cast<int8_t*>(output.getData()), output.getSize(), k);
}

std::string workerLoad(Client* client, ParameterMap* parameters) {
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
  const std::array golden{259, 261, 260, 157, 230};
  const auto k = golden.size();

  for (const auto& response : responses) {
    auto outputs = response.getOutputs();
    ASSERT_EQ(outputs.size(), 1);
    auto top_k = postprocess(outputs[0], k);
    for (auto j = 0U; j < k; ++j) {
      EXPECT_EQ(top_k.at(j), golden.at(j));
    }
  }
}

void test0(Client* client) {
  if (!serverHasExtension(client, "vitis")) {
    GTEST_SKIP() << "Vitis AI support required from the server but not found";
  }

  if (!client->hasHardware("DPUCADF8H", 1)) {
    GTEST_SKIP() << "At least one DPUCADF8H required on the server";
  }

  const auto test_asset = getPathToAsset("asset_dog-3619020_640.jpg");
  const auto xmodel = getPathToAsset("u250_resnet50");

  amdinfer::ParameterMap parameters;
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
TEST_F(GrpcFixture, WorkersXmodelResnet50) { test0(client_.get()); }
#endif

// @pytest.mark.extensions(["vitis"])
// @pytest.mark.fpgas("DPUCADF8H", 1)
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, WorkersXmodelResnet50) {
  NativeClient client(&server_);
  test0(&client);
}

#ifdef AMDINFER_ENABLE_HTTP
// @pytest.mark.extensions(["vitis"])
// @pytest.mark.fpgas("DPUCADF8H", 1)
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, WorkersXmodelResnet50) { test0(client_.get()); }
#endif

}  // namespace amdinfer
