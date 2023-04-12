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
 * @brief Performance testing for resnet50
 */

#include <benchmark/benchmark.h>

#include <algorithm>            // for copy, max
#include <array>                // for array
#include <cassert>              // for assert
#include <chrono>               // for duration, operator-
#include <cstdint>              // for uint64_t
#include <cstdlib>              // for getenv
#include <filesystem>           // for path, operator/
#include <iostream>             // for operator<<, basic_...
#include <memory>               // for unique_ptr, alloca...
#include <opencv2/core.hpp>     // for CV_32FC3
#include <opencv2/imgproc.hpp>  // for COLOR_BGR2RGB
#include <ratio>                // for milli
#include <string>               // for string, allocator
#include <thread>               // for sleep_for
#include <tuple>                // for tuple, _Swallow_as...
#include <utility>              // for forward
#include <variant>              // for variant, visit
#include <vector>               // for vector

#include "amdinfer/amdinfer.hpp"                   // for InferenceRequest
#include "amdinfer/pre_post/image_preprocess.hpp"  // for ImagePreprocessOpt...
#include "amdinfer/testing/get_path_to_asset.hpp"  // for getPathToAsset

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

const std::array<Config, 4> kConfigs = {Config{1, 10}, Config{2, 20},
                                        Config{20, 40}, Config{40, 40}};

template <typename T, int kChannels>
using ImagePreprocessOptions =
  amdinfer::pre_post::ImagePreprocessOptions<T, kChannels>;

struct Workers {
  char* root = std::getenv("AMDINFER_ROOT");
  std::string extension;
  std::string name;
  fs::path graph;

  amdinfer::ParameterMap parameters;
  std::variant<ImagePreprocessOptions<float, 3>> preprocessing;
};

struct PtzendnnWorker : public Workers {
  PtzendnnWorker() {
    assert(root != nullptr);
    extension = "ptzendnn";
    name = "ptzendnn";
    graph =
      fs::path{root} / "external/artifacts/resnet50/resnet50_pretrained.pt";

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
            "external/artifacts/resnet50/resnet_v1_50_baseline_6.96B_922.pb";

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

// NOLINTNEXTLINE(cert-err58-cpp)
const PtzendnnWorker kPtZendnn{};
// NOLINTNEXTLINE(cert-err58-cpp)
const TfzendnnWorker kTfZendnn{};

const std::array<const Workers*, 2> kWorkers{&kPtZendnn, &kTfZendnn};

template <class... Fs>
struct Overload : Fs... {
  using Fs::operator()...;
};
template <class... Fs>
Overload(Fs...) -> Overload<Fs...>;

template <class... Args>
void resnet50(benchmark::State& st, Args&&... args) {
  auto args_tuple = std::make_tuple(std::forward<Args>(args)...);
  int client_index = std::get<0>(args_tuple);

  auto config_index = st.range(0);
  auto worker_index = st.range(1);
  const auto& config = kConfigs.at(config_index);
  const auto* worker = kWorkers.at(worker_index);

  const auto image_location =
    amdinfer::getPathToAsset("asset_dog-3619020_640.jpg");
  const auto input_size = 224;
  const auto channels = 3;
  const auto output_classes = 1000;
  const auto batch_size = config.batch_size;

  const auto warmup_requests = 4;

  const auto& name = worker->name;

  auto parameters = worker->parameters;
  parameters.put("input_size", input_size);
  parameters.put("output_classes", output_classes);
  parameters.put("batch_size", batch_size);

  [[maybe_unused]] const auto default_http_port = 8998;
  [[maybe_unused]] const auto default_grpc_port = 50'051;

  amdinfer::Server server;
  std::unique_ptr<amdinfer::Client> client;
  if (client_index == 0) {
    client = std::make_unique<amdinfer::NativeClient>(&server);
#ifdef AMDINFER_ENABLE_HTTP
  } else if (client_index == 1) {
    server.startHttp(default_http_port);
    client = std::make_unique<amdinfer::HttpClient>("http://127.0.0.1:8998");
#endif
#ifdef AMDINFER_ENABLE_GRPC
  } else if (client_index == 2) {
    server.startGrpc(default_grpc_port);
    client = std::make_unique<amdinfer::GrpcClient>("127.0.0.1:50051");
#endif
  } else {
    st.SkipWithError("Unknown client index passed");
    return;
  }

  amdinfer::waitUntilServerReady(client.get());

  auto endpoint = client->workerLoad(name, parameters);
  assert(endpoint == name);

  std::vector<std::string> paths{image_location};
  const auto& options = worker->preprocessing;

  amdinfer::InferenceRequest request;
  std::vector<std::vector<float>> images;
  std::visit(
    Overload{
      [&](const ImagePreprocessOptions<float, channels>& opts) {
        images = amdinfer::pre_post::imagePreprocess(paths, opts);
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

  for (auto _ : st) {
    if (!amdinfer::serverHasExtension(client.get(), worker->extension)) {
      std::string error = worker->extension + " support required but not found";
      st.SkipWithError(error.c_str());
      return;
    }

    auto num_requests = config.requests;
    for (auto i = 0; i < num_requests; ++i) {
      auto response = client->modelInfer(endpoint, request);
      assert(!response.isError());
    }
  }
  client->workerUnload(endpoint);
  while (client->modelReady(endpoint)) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

// NOLINTNEXTLINE(cert-err58-cpp)
const std::initializer_list<std::vector<int64_t>> kRange{
  benchmark::CreateDenseRange(0, kConfigs.size() - 1, 1),
  benchmark::CreateDenseRange(0, kWorkers.size() - 1, 1)};

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
BENCHMARK_CAPTURE(resnet50, Native, 0)
  ->ArgsProduct(kRange)
  ->Unit(benchmark::kMillisecond);
#ifdef AMDINFER_ENABLE_HTTP
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
BENCHMARK_CAPTURE(resnet50, HTTP, 1)
  ->ArgsProduct(kRange)
  ->Unit(benchmark::kMillisecond);
#endif
#ifdef AMDINFER_ENABLE_GRPC
// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
BENCHMARK_CAPTURE(resnet50, gRPC, 2)
  ->ArgsProduct(kRange)
  ->Unit(benchmark::kMillisecond);
#endif

// NOLINTNEXTLINE
BENCHMARK_MAIN();
