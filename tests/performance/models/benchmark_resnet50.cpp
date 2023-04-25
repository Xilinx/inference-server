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

using InferenceRequest = amdinfer::InferenceRequest;
using ParameterMap = amdinfer::ParameterMap;

class Config {
 public:
  int batch_size;
  int requests;

  std::string toString() const {
    return "batch_size:" + std::to_string(batch_size) +
           "/requests:" + std::to_string(requests);
  }

  friend std::ostream& operator<<(std::ostream& os, const Config& self) {
    os << self.toString();
    return os;
  }
};

const std::array kConfigs{Config{1, 1}, Config{4, 4}, Config{4, 8}};

class Backend {
 public:
  virtual ~Backend() = default;

  virtual void preprocess(const std::string& input_path) = 0;

  virtual std::string extension() const = 0;
  virtual std::string name() const = 0;

  void request(InferenceRequest request) { request_ = std::move(request); }
  const InferenceRequest& request() const { return request_; }

  const auto& parameters() const { return parameters_; }
  void put(const std::string& key, const amdinfer::Parameter& value) {
    parameters_.put(key, value);
  }

 private:
  InferenceRequest request_;
  ParameterMap parameters_;
};

class Ptzendnn : public Backend {
 public:
  Ptzendnn() {
    auto model = amdinfer::getPathToAsset("pt_resnet50");

    this->put("model", model);
  }

  std::string name() const override { return "ptzendnn"; }
  std::string extension() const override { return "ptzendnn"; }

  void preprocess(const std::string& input_path) override {
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

    images_ = amdinfer::pre_post::imagePreprocess({input_path}, options);

    InferenceRequest request;
    request.addInputTensor(images_[0].data(), {3, 224, 224},
                           amdinfer::DataType::Fp32);
    this->request(std::move(request));
  }

 private:
  std::vector<std::vector<float>> images_;
};

class Tfzendnn : public Backend {
 public:
  Tfzendnn() {
    // arbitrarily set to 64
    const int inter_op = 64;

    auto model = amdinfer::getPathToAsset("tf_resnet50");

    put("model", model);
    put("input_node", std::string{"input"});
    put("output_node", std::string{"resnet_v1_50/predictions/Reshape_1"});
    put("input_size", 224);
    put("output_classes", 1000);
    put("inter_op", inter_op);
    put("intra_op", 1);
  }

  std::string name() const override { return "tfzendnn"; }
  std::string extension() const override { return "tfzendnn"; }

  void preprocess(const std::string& input_path) override {
    amdinfer::pre_post::ImagePreprocessOptions<float, 3> options;
    options.convert_color = true;
    options.color_code = cv::COLOR_BGR2RGB;
    options.assign = true;

    images_ = amdinfer::pre_post::imagePreprocess({input_path}, options);

    InferenceRequest request;
    request.addInputTensor(images_[0].data(), {3, 224, 224},
                           amdinfer::DataType::Fp32);
    this->request(std::move(request));
  }

 private:
  std::vector<std::vector<float>> images_;
};

class Vitis : public Backend {
 public:
  Vitis() {
    auto model = amdinfer::getPathToAsset("u250_resnet50");
    put("model", model);
  }

  std::string name() const override { return "xmodel"; }
  std::string extension() const override { return "vitis"; }

  void preprocess(const std::string& input_path) override {
    const std::array<int8_t, 3> mean{123, 107, 104};
    const std::array<int8_t, 3> std{1, 1, 1};

    amdinfer::pre_post::ImagePreprocessOptions<int8_t, 3> options;
    options.order = amdinfer::pre_post::ImageOrder::NHWC;
    options.mean = mean;
    options.std = std;
    options.normalize = true;
    images_ = amdinfer::pre_post::imagePreprocess({input_path}, options);

    InferenceRequest request;
    request.addInputTensor(images_[0].data(), {224, 224, 3},
                           amdinfer::DataType::Int8);
    this->request(std::move(request));
  }

 private:
  std::vector<std::vector<int8_t>> images_;
};

class Migraphx : public Backend {
 public:
  Migraphx() {
    auto model = amdinfer::getPathToAsset("onnx_resnet50");
    put("model", model);
  }

  std::string name() const override { return "migraphx"; }
  std::string extension() const override { return "migraphx"; }

  void preprocess(const std::string& input_path) override {
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
    images_ = amdinfer::pre_post::imagePreprocess({input_path}, options);

    InferenceRequest request;
    request.addInputTensor(images_[0].data(), {224, 224, 3},
                           amdinfer::DataType::Fp32);
    this->request(std::move(request));
  }

 private:
  std::vector<std::vector<float>> images_;
};

const auto kNumBackends = 1;
enum class Backends {
  // Tfzendnn,
  // Ptzendnn,
  Vitis,
  // Migraphx
};

std::unique_ptr<Backend> getBackend(Backends index) {
  switch (index) {
    // case Backends::Tfzendnn:
    //   return std::make_unique<Tfzendnn>();
    // case Backends::Ptzendnn:
    //   return std::make_unique<Ptzendnn>();
    case Backends::Vitis:
      return std::make_unique<Vitis>();
    // case Backends::Migraphx:
    //   return std::make_unique<Migraphx>();
    default:
      throw amdinfer::invalid_argument("Unknown argument");
  }
}

// the benchmark function cannot be part of a namespace

// template <class... Args>
void resnet50(benchmark::State& st, const amdinfer::Client* client,
              Backend* backend) {
  const auto extension = backend->extension();
  if (!amdinfer::serverHasExtension(client, extension)) {
    std::string error = extension + " support required but not found";
    st.SkipWithError(error.c_str());
    return;
  }

  auto config_index = st.range(0);
  const auto& config = kConfigs.at(config_index);
  st.SetLabel(config.toString());

  const auto num_requests = config.requests;
  const auto batch_size = config.batch_size;

  backend->put("batch_size", batch_size);

  const auto& request = backend->request();
  const auto& parameters = backend->parameters();

  const auto& name = backend->name();
  auto endpoint = client->workerLoad(name, parameters);
  assert(endpoint == name);

  const auto warmup_requests = 4;
  for (auto i = 0; i < warmup_requests; ++i) {
    auto response = client->modelInfer(endpoint, request);
    if (response.isError()) {
      st.SkipWithError("Error response from the server");
      return;
    }
  }

  // const size_t batches = num_requests / batch_size;
  // const size_t leftover_requests = num_requests % batch_size;

  const std::vector<amdinfer::InferenceRequest> requests{
    static_cast<size_t>(num_requests), request};
  // const std::vector<amdinfer::InferenceRequest>
  // extra_requests{leftover_requests, request};

  for (auto _ : st) {
    // for (auto i = 0U; i < batches; ++i) {
    std::ignore = amdinfer::inferAsyncOrdered(client, endpoint, requests);
    // for(auto i = 0; i < num_requests; ++i){
    //   std::ignore = client->modelInfer(endpoint, request);
    // }
    // }
    // std::ignore = amdinfer::inferAsyncOrderedBatched(client, endpoint,
    // extra_requests, batch_size);
  }
  client->workerUnload(endpoint);
  amdinfer::waitUntilModelNotReady(client, endpoint);
}

// NOLINTNEXTLINE(cert-err58-cpp)
const std::initializer_list<std::vector<int64_t>> kRange{
  benchmark::CreateDenseRange(0, kConfigs.size() - 1, 1)};

int main(int argc, char* argv[]) {
  const amdinfer::Server server;

#if defined(PROTOCOL_HTTP)
  const auto default_http_port = 8998;
  server.startHttp(default_http_port);
  auto client = std::make_unique<amdinfer::HttpClient>(
    "http://127.0.0.1:" + std::to_string(default_http_port));
#elif defined(PROTOCOL_GRPC)
  const auto default_grpc_port = 50051;
  server.startGrpc(default_grpc_port);
  auto client = std::make_unique<amdinfer::GrpcClient>(
    "127.0.0.1:" + std::to_string(default_grpc_port));
#else
  auto client = std::make_unique<amdinfer::NativeClient>(&server);
#endif

  std::vector<std::unique_ptr<Backend>> backends;
  backends.reserve(kNumBackends);
  const auto image_location =
    amdinfer::getPathToAsset("asset_dog-3619020_640.jpg");

  for (auto i = 0; i < kNumBackends; ++i) {
    backends.push_back(getBackend(static_cast<Backends>(i)));
    auto* backend = backends.back().get();
    auto name = "ResNet50/" + backend->name();
    auto* benchmark = benchmark::RegisterBenchmark(name.c_str(), resnet50,
                                                   client.get(), backend);
    benchmark->ArgsProduct(kRange);
    benchmark->Unit(benchmark::kMillisecond);

    // construct request
    std::vector<std::string> paths{image_location};
    backend->preprocess(image_location);
  }

  benchmark::Initialize(&argc, argv);
  amdinfer::waitUntilServerReady(client.get());
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
}
