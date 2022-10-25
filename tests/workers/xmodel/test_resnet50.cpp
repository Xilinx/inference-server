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

#include <string>
#include <vector>

#include "proteus/clients/client.hpp"
#include "proteus/pre_post/image_preprocess.hpp"
#include "proteus/pre_post/resnet50_postprocess.hpp"
#include "proteus/testing/get_path_to_asset.hpp"
#include "proteus/testing/gtest_fixtures.hpp"

namespace proteus {

using Images = std::vector<std::vector<int8_t>>;

Images preprocess(const std::vector<std::string>& paths) {
  proteus::pre_post::ImagePreprocessOptions<int8_t, 3> options;
  options.order = proteus::pre_post::ImageOrder::NHWC;
  options.mean = {123, 107, 104};
  options.std = {1, 1, 1};
  options.normalize = true;
  return pre_post::imagePreprocess(paths, options);
}

std::vector<int> postprocess(const proteus::InferenceResponseOutput& output,
                             int k) {
  return proteus::pre_post::resnet50Postprocess(
    static_cast<int8_t*>(output.getData()), output.getSize(), k);
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
    requests.back().addInputTensor(const_cast<int8_t*>(image.data()), shape,
                                   DataType::INT8);
  }

  return requests;
}

void validate(const std::vector<InferenceResponse>& responses) {
  const std::array golden{259, 261, 260, 157, 230};
  const auto k = golden.size();

  for (const auto& response : responses) {
    auto outputs = response.getOutputs();
    EXPECT_EQ(outputs.size(), 1);
    auto top_k = postprocess(outputs[0], k);
    for (auto j = 0U; j < k; ++j) {
      EXPECT_EQ(top_k[j], golden[j]);
    }
  }
}

void test_0(Client* client) {
  if (!serverHasExtension(client, "vitis")) {
    GTEST_SKIP() << "Vitis AI support required from the server but not found";
  }

  if (!client->hasHardware("DPUCADF8H", 1)) {
    GTEST_SKIP() << "At least one DPUCADF8H required on the server";
  }

  const auto kTestAsset = getPathToAsset("asset_dog-3619020_640.jpg");
  const auto kXmodel = getPathToAsset("u250_resnet50");

  proteus::RequestParameters parameters;
  parameters.put("model", kXmodel);

  auto images = preprocess({kTestAsset});
  auto endpoint = workerLoad(client, &parameters);
  auto requests = constructRequests(images);
  auto responses = inferAsyncOrdered(client, endpoint, requests);
  validate(responses);
}

#ifdef PROTEUS_ENABLE_GRPC
// @pytest.mark.extensions(["vitis"])
// @pytest.mark.fpgas("DPUCADF8H", 1)
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(GrpcFixture, WorkersXmodelResnet50) { test_0(client_.get()); }
#endif

// @pytest.mark.extensions(["vitis"])
// @pytest.mark.fpgas("DPUCADF8H", 1)
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(BaseFixture, WorkersXmodelResnet50) {
  NativeClient client;
  test_0(&client);
}

#ifdef PROTEUS_ENABLE_HTTP
// @pytest.mark.extensions(["vitis"])
// @pytest.mark.fpgas("DPUCADF8H", 1)
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_F(HttpFixture, WorkersXmodelResnet50) { test_0(client_.get()); }
#endif

}  // namespace proteus
