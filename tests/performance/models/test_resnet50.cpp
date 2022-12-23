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

/**
 * @file
 * @brief Performance testing for the native client
 */

#include <chrono>
#include <filesystem>
#include <queue>
#include <string>
#include <vector>

#include "amdinfer/amdinfer.hpp"  // for InferenceResponse, Grp...
#include "amdinfer/pre_post/image_preprocess.hpp"
#include "amdinfer/testing/get_path_to_asset.hpp"  // for getPathToAsset
#include "amdinfer/testing/gtest_fixtures.hpp"     // for GrpcFixture
#include "amdinfer/util/ctpl.hpp"                  // for thread_pool

namespace fs = std::filesystem;

struct Config {
  int batch_size;
  int requests;

  friend std::ostream& operator<<(std::ostream& os, const Config& self) {
    os << "Batch Size: " << self.batch_size << ", ";
    os << "Requests: " << self.requests << ", ";
    return os;
  }
};

template <typename T, int kChannels>
using ImagePreprocessOptions =
  amdinfer::pre_post::ImagePreprocessOptions<T, kChannels>;

struct Workers {
  char* root = std::getenv("AMDINFER_ROOT");
  std::string extension;
  std::string name;
  fs::path graph;

  amdinfer::RequestParameters parameters;
  std::variant<ImagePreprocessOptions<float, 3>> preprocessing;
};

struct PtzendnnWorker : public Workers {
  PtzendnnWorker() {
    assert(root != nullptr);
    extension = "ptzendnn";
    name = "ptzendnn";
    graph = fs::path{root} / "external/pytorch_models/resnet50_pretrained.pt";

    parameters.put("model", graph.string());

    const std::array<float, 3> mean{0.485F, 0.456F, 0.406F};
    const std::array<float, 3> std{4.367F, 4.464F, 4.444F};
    const auto convert_scale = 1 / 255.0;

    amdinfer::pre_post::ImagePreprocessOptions<float, 3> options;
    options.normalize = true;
    options.order = amdinfer::pre_post::ImageOrder::NCHW;
    options.mean = mean;
    options.std = std;
    options.convert_color = true;
    options.color_code = cv::COLOR_BGR2RGB;
    options.convert_type = true;
    options.type = CV_32FC3;
    options.convert_scale = convert_scale;
    preprocessing = options;
  }
};

struct TfzendnnWorker : public Workers {
  TfzendnnWorker() {
    assert(root != nullptr);
    extension = "tfzendnn";
    name = "tfzendnn";
    graph = fs::path{root} /
            "external/tensorflow_models/resnet_v1_50_baseline_6.96B_922.pb";

    // arbitrarily set to 64
    const int inter_op = 64;

    parameters.put("model", graph.string());
    parameters.put("input_node", "input");
    parameters.put("output_node", "resnet_v1_50/predictions/Reshape_1");
    parameters.put("inter_op", inter_op);
    parameters.put("intra_op", 1);

    amdinfer::pre_post::ImagePreprocessOptions<float, 3> options;
    options.convert_color = true;
    options.color_code = cv::COLOR_BGR2RGB;
    options.assign = true;
    preprocessing = options;
  }
};

class PerfModelsResnetBaseFixture
  : public BaseFixtureWithParams<std::tuple<Config, const Workers*>> {};
class PerfModelsResnetHttpFixture
  : public HttpFixtureWithParams<std::tuple<Config, const Workers*>> {};
class PerfModelsResnetGrpcFixture
  : public GrpcFixtureWithParams<std::tuple<Config, const Workers*>> {};

template <class... Fs>
struct Overload : Fs... {
  using Fs::operator()...;
};
template <class... Fs>
Overload(Fs...) -> Overload<Fs...>;

void test(amdinfer::Client* client, const Config& config,
          const Workers* worker) {
  if (!amdinfer::serverHasExtension(client, worker->extension)) {
    GTEST_SKIP() << worker->extension << " support required but not found.\n";
  }

  const auto image_location =
    amdinfer::getPathToAsset("asset_dog-3619020_640.jpg");
  const auto input_size = 224;
  const auto channels = 3;
  const auto output_classes = 1000;
  const auto batch_size = config.batch_size;

  const auto warmup_requests = 4;
  const auto num_requests = config.requests;

  const auto& name = worker->name;

  auto parameters = worker->parameters;
  parameters.put("input_size", input_size);
  parameters.put("output_classes", output_classes);
  parameters.put("batch_size", batch_size);

  auto endpoint = client->workerLoad(name, &parameters);
  EXPECT_EQ(endpoint, name);

  std::vector<std::string> paths{image_location};
  const auto& options = worker->preprocessing;

  amdinfer::InferenceRequest request;
  std::visit(
    Overload{
      [&](const ImagePreprocessOptions<float, channels>& opts) {
        auto images = amdinfer::pre_post::imagePreprocess(paths, opts);
        request.addInputTensor(images[0].data(),
                               {channels, input_size, input_size},
                               amdinfer::DataType::Fp32);
      },
    },
    options);

  // warm up
  for (auto i = 0; i < warmup_requests; ++i) {
    std::ignore = client->modelInfer(endpoint, request);
  }

  auto start = std::chrono::high_resolution_clock::now();
  for (auto i = 0; i < num_requests; ++i) {
    auto response = client->modelInfer(endpoint, request);
    EXPECT_FALSE(response.isError());
  }
  auto stop = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> duration = stop - start;
  std::cout << "-----\n";
  std::cout << "sync time taken for " << num_requests
            << " images with batch size " << batch_size << " on " << name
            << ": " << duration.count() << "ms" << std::endl;
  std::cout << "-----\n";

  std::vector<amdinfer::InferenceRequest> requests;
  requests.reserve(num_requests);
  for (auto i = 0; i < num_requests; ++i) {
    requests.push_back(request);
  }
  start = std::chrono::high_resolution_clock::now();
  auto responses = amdinfer::inferAsyncOrdered(client, endpoint, requests);
  stop = std::chrono::high_resolution_clock::now();
  duration = stop - start;
  std::cout << "-----\n";
  std::cout << "async time taken for " << num_requests
            << " images with batch size " << batch_size << " on " << name
            << ": " << duration.count() << "ms" << std::endl;
  std::cout << "-----\n";

  client->workerUnload(endpoint);
  while (client->modelReady(endpoint)) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

const std::array<Config, 4> kConfigs = {Config{1, 10}, Config{2, 20},
                                        Config{20, 40}, Config{40, 40}};

// NOLINTNEXTLINE(cert-err58-cpp)
const PtzendnnWorker kPtZendnn{};
// NOLINTNEXTLINE(cert-err58-cpp)
const TfzendnnWorker kTfZendnn{};

const std::array<const Workers*, 1> kWorkers = {
  // TODO(amuralee): why does TFzendnn slow down dramatically if included
  // together
  //  &kPtZendnn, &kTfZendnn
  &kTfZendnn};

#ifdef AMDINFER_ENABLE_GRPC

// @pytest.mark.perf(group="clients")
TEST_P(PerfModelsResnetGrpcFixture, ModelInfer) {  // NOLINT
  const auto& [config, worker] = GetParam();
  test(client_.get(), config, worker);
}

// NOLINTNEXTLINE(cert-err58-cpp)
INSTANTIATE_TEST_SUITE_P(PerfModelsResnetGrpc, PerfModelsResnetGrpcFixture,
                         testing::Combine(testing::ValuesIn(kConfigs),
                                          testing::ValuesIn(kWorkers)));
#endif

// @pytest.mark.perf(group="clients")
TEST_P(PerfModelsResnetBaseFixture, ModelInfer) {  // NOLINT
  amdinfer::NativeClient client;
  const auto& [config, worker] = GetParam();
  test(&client, config, worker);
}

// NOLINTNEXTLINE(cert-err58-cpp)
INSTANTIATE_TEST_SUITE_P(PerfModelsResnetBase, PerfModelsResnetBaseFixture,
                         testing::Combine(testing::ValuesIn(kConfigs),
                                          testing::ValuesIn(kWorkers)));

#ifdef AMDINFER_ENABLE_HTTP
// @pytest.mark.perf(group="clients")
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_P(PerfModelsResnetHttpFixture, ModelInfer) {  // NOLINT
  const auto& [config, worker] = GetParam();
  test(client_.get(), config, worker);
}

// NOLINTNEXTLINE(cert-err58-cpp)
INSTANTIATE_TEST_SUITE_P(PerfModelsResnetHttp, PerfModelsResnetHttpFixture,
                         testing::Combine(testing::ValuesIn(kConfigs),
                                          testing::ValuesIn(kWorkers)));
#endif
