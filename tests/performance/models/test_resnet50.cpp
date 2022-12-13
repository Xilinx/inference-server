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
#include "amdinfer/util/ctpl.h"                    // for thread_pool

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

template <typename T, int C>
using ImagePreprocessOptions = amdinfer::pre_post::ImagePreprocessOptions<T, C>;

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
    extension = "ptzendnn";
    name = "ptzendnn";
    if (root != nullptr) {
      graph = fs::path{root} / "external/pytorch_models/resnet50_pretrained.pt";
    } else {
      throw amdinfer::environment_not_set_error("AMDINFER_ROOT not set");
    }

    parameters.put("model", graph.string());

    amdinfer::pre_post::ImagePreprocessOptions<float, 3> options;
    options.normalize = true;
    options.order = amdinfer::pre_post::ImageOrder::NCHW;
    options.mean = {0.485, 0.456, 0.406};
    options.std = {4.367, 4.464, 4.444};
    options.convert_color = true;
    options.color_code = cv::COLOR_BGR2RGB;
    options.convert_type = true;
    options.type = CV_32FC3;
    options.convert_scale = 1.0 / 255.0;
    preprocessing = options;
  }
};

struct TfzendnnWorker : public Workers {
  TfzendnnWorker() {
    extension = "tfzendnn";
    name = "tfzendnn";
    if (root != nullptr) {
      graph = fs::path{root} /
              "external/tensorflow_models/resnet_v1_50_baseline_6.96B_922.pb";
    } else {
      throw amdinfer::environment_not_set_error("AMDINFER_ROOT not set");
    }

    parameters.put("model", graph.string());
    parameters.put("input_node", "input");
    parameters.put("output_node", "resnet_v1_50/predictions/Reshape_1");
    parameters.put("inter_op", 64);
    parameters.put("intra_op", 1);

    amdinfer::pre_post::ImagePreprocessOptions<float, 3> options;
    options.convert_color = true;
    options.color_code = cv::COLOR_BGR2RGB;
    options.assign = true;
    preprocessing = options;
  }
};

class PerfModelsResnetBaseFixture
  : public BaseFixtureWithParams<std::tuple<Config, Workers*>> {};
class PerfModelsResnetHttpFixture
  : public HttpFixtureWithParams<std::tuple<Config, Workers*>> {};
class PerfModelsResnetGrpcFixture
  : public GrpcFixtureWithParams<std::tuple<Config, Workers*>> {};

template <class... Fs>
struct Overload : Fs... {
  using Fs::operator()...;
};
template <class... Fs>
Overload(Fs...) -> Overload<Fs...>;

void test(amdinfer::Client* client, const Config& config, Workers* worker) {
  if (!amdinfer::serverHasExtension(client, worker->extension)) {
    GTEST_SKIP() << worker->extension << " support required but not found.\n";
  }

  const auto kImageLocation =
    amdinfer::getPathToAsset("asset_dog-3619020_640.jpg");
  const auto kInputSize = 224;
  const auto kOutputClasses = 1000;
  const auto kBatchSize = config.batch_size;

  const auto kWarmupRequests = 4;
  const auto kRequests = config.requests;

  const auto& name = worker->name;

  auto& parameters = worker->parameters;
  parameters.put("input_size", kInputSize);
  parameters.put("output_classes", kOutputClasses);
  parameters.put("batch_size", kBatchSize);

  auto endpoint = client->workerLoad(name, &parameters);
  EXPECT_EQ(endpoint, name);

  std::vector<std::string> paths{kImageLocation};
  auto& options = worker->preprocessing;

  amdinfer::InferenceRequest request;
  std::visit(
    Overload{
      [&](const ImagePreprocessOptions<float, 3>& options_) {
        auto images = amdinfer::pre_post::imagePreprocess(paths, options_);
        request.addInputTensor(images[0].data(), {3, kInputSize, kInputSize},
                               amdinfer::DataType::FLOAT32);
      },
    },
    options);

  // warm up
  for (auto i = 0; i < kWarmupRequests; ++i) {
    client->modelInfer(endpoint, request);
  }

  auto start = std::chrono::high_resolution_clock::now();
  for (auto i = 0; i < kRequests; ++i) {
    auto response = client->modelInfer(endpoint, request);
    EXPECT_FALSE(response.isError());
  }
  auto stop = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> duration = stop - start;
  std::cout << "-----\n";
  std::cout << "sync time taken for " << kRequests << " images with batch size "
            << kBatchSize << " on " << name << ": " << duration.count() << "ms"
            << std::endl;
  std::cout << "-----\n";

  std::vector<amdinfer::InferenceRequest> requests;
  requests.reserve(kRequests);
  for (auto i = 0; i < kRequests; ++i) {
    requests.push_back(request);
  }
  start = std::chrono::high_resolution_clock::now();
  auto responses = amdinfer::inferAsyncOrdered(client, endpoint, requests);
  stop = std::chrono::high_resolution_clock::now();
  duration = stop - start;
  std::cout << "-----\n";
  std::cout << "async time taken for " << kRequests
            << " images with batch size " << kBatchSize << " on " << name
            << ": " << duration.count() << "ms" << std::endl;
  std::cout << "-----\n";

  client->workerUnload(endpoint);
  while (client->modelReady(endpoint)) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

const std::array<Config, 4> configs = {Config{1, 10}, Config{2, 20},
                                       Config{20, 40}, Config{40, 40}};

inline static PtzendnnWorker ptzendnn;
inline static TfzendnnWorker tfzendnn;

const std::array<Workers*, 1> workers = {
  // TODO(amuralee): why does TFzendnn slow down dramatically if included
  // together
  //  &ptzendnn, &tfzendnn
  &tfzendnn};

#ifdef AMDINFER_ENABLE_GRPC

// @pytest.mark.perf(group="clients")
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_P(PerfModelsResnetGrpcFixture, ModelInfer) {
  const auto& [config, worker] = GetParam();
  test(client_.get(), config, worker);
}

INSTANTIATE_TEST_SUITE_P(PerfModelsResnetGrpc, PerfModelsResnetGrpcFixture,
                         testing::Combine(testing::ValuesIn(configs),
                                          testing::ValuesIn(workers)));
#endif

// @pytest.mark.perf(group="clients")
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_P(PerfModelsResnetBaseFixture, ModelInfer) {
  amdinfer::NativeClient client;
  const auto& [config, worker] = GetParam();
  test(&client, config, worker);
}

INSTANTIATE_TEST_SUITE_P(PerfModelsResnetBase, PerfModelsResnetBaseFixture,
                         testing::Combine(testing::ValuesIn(configs),
                                          testing::ValuesIn(workers)));

#ifdef AMDINFER_ENABLE_HTTP
// @pytest.mark.perf(group="clients")
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST_P(PerfModelsResnetHttpFixture, ModelInfer) {
  const auto& [config, worker] = GetParam();
  test(client_.get(), config, worker);
}

INSTANTIATE_TEST_SUITE_P(PerfModelsResnetHttp, PerfModelsResnetHttpFixture,
                         testing::Combine(testing::ValuesIn(configs),
                                          testing::ValuesIn(workers)));
#endif