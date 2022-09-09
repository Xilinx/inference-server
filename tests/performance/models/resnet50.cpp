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
using ImagePreprocessOptions = proteus::util::ImagePreprocessOptions<T, C>;

struct Workers {
  const fs::path kRoot{std::getenv("PROTEUS_ROOT")};
  std::string name;
  fs::path graph;

  proteus::RequestParameters parameters;
  std::variant<ImagePreprocessOptions<float, 3>> preprocessing;
};

struct PtzendnnWorker : public Workers {
  PtzendnnWorker() {
    name = "ptzendnn";
    graph = kRoot / "external/pytorch_models/resnet50_pretrained.pt";

    parameters.put("model", graph.string());

    proteus::util::ImagePreprocessOptions<float, 3> options;
    options.normalize = true;
    options.order = proteus::util::ImageOrder::NCHW;
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
    name = "tfzendnn";
    graph =
      kRoot / "external/tensorflow_models/resnet_v1_50_baseline_6.96B_922.pb";

    parameters.put("model", graph.string());
    parameters.put("input_node", "input");
    parameters.put("output_node", "resnet_v1_50/predictions/Reshape_1");
    parameters.put("inter_op", 64);
    parameters.put("intra_op", 1);

    proteus::util::ImagePreprocessOptions<float, 3> options;
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

void test(proteus::Client* client, const Config& config, Workers* worker) {
  const fs::path kRoot{std::getenv("PROTEUS_ROOT")};

  const auto kImageLocation = kRoot / "tests/assets/dog-3619020_640.jpg";
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

  proteus::InferenceRequest request;
  std::visit(
    Overload{
      [&](const ImagePreprocessOptions<float, 3>& options_) {
        auto images = proteus::util::imagePreprocess(paths, options_);
        request.addInputTensor(images[0].data(), {3, kInputSize, kInputSize},
                               proteus::DataType::FLOAT32);
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

  std::vector<proteus::InferenceRequest> requests;
  requests.reserve(kRequests);
  for (auto i = 0; i < kRequests; ++i) {
    requests.push_back(request);
  }
  start = std::chrono::high_resolution_clock::now();
  auto responses = proteus::inferAsyncOrdered(client, endpoint, requests);
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

#ifdef PROTEUS_ENABLE_GRPC
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
  proteus::NativeClient client;
  const auto& [config, worker] = GetParam();
  test(&client, config, worker);
}

INSTANTIATE_TEST_SUITE_P(PerfModelsResnetBase, PerfModelsResnetBaseFixture,
                         testing::Combine(testing::ValuesIn(configs),
                                          testing::ValuesIn(workers)));

#ifdef PROTEUS_ENABLE_HTTP
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
