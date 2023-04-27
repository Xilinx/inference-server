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

/**
 * @brief This class wraps the different parameters used by these benchmarks.
 * The benchmark library works by parameterizing numbers so if you want to pass
 * other kinds of data to the tests, you need to wrap it in a class.
 */
class Config {
 public:
  Config(int batch_size, int requests, int workers)
    : batch_size_({batch_size, batch_size}),
      requests_(requests),
      workers_(workers) {}

  int requests() const { return requests_; }
  int batchSize() const { return batch_size_.first; }
  void batchSize(int batch_size) { batch_size_.second = batch_size; }
  int workers() const { return workers_; }

  std::string toString() const {
    return "batch_size:" + std::to_string(batch_size_.second) + "(" +
           std::to_string(batch_size_.first) +
           ")/requests:" + std::to_string(requests_) +
           "/workers:" + std::to_string(workers_);
  }

  friend std::ostream& operator<<(std::ostream& os, const Config& self) {
    os << self.toString();
    return os;
  }

 private:
  // This is a pair representing the <requested, actual> value of the batch
  // size. A particular backend may not support the requested batch size and
  // use a different value
  std::pair<int, int> batch_size_;
  int requests_;
  int workers_;
};

class Backend {
 public:
  virtual ~Backend() = default;

  virtual void preprocess(const std::string& input_path) = 0;

  virtual std::string extension() const = 0;
  virtual std::string name() const = 0;
  virtual void updateConfig(Config& config) = 0;

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

const auto kImageHeight = 224;
const auto kImageWidth = 224;
const auto kImageChannels = 3;
// for ImageNet dataset, 1000 output classes
const auto kOutputClasses = 1000;

#ifdef AMDINFER_ENABLE_PTZENDNN
class Ptzendnn : public Backend {
 public:
  Ptzendnn() {
    auto model = amdinfer::getPathToAsset("pt_resnet50");

    this->put("model", model);
  }

  std::string name() const override { return "ptzendnn"; }
  std::string extension() const override { return "ptzendnn"; }
  void updateConfig([[maybe_unused]] Config& config) override {
    // no update necessary
  }

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
    request.addInputTensor(images_[0].data(),
                           {kImageChannels, kImageHeight, kImageWidth},
                           amdinfer::DataType::Fp32);
    this->request(std::move(request));
  }

 private:
  std::vector<std::vector<float>> images_;
};
#endif  // AMDINFER_ENABLE_PTZENDNN

#ifdef AMDINFER_ENABLE_TFZENDNN
class Tfzendnn : public Backend {
 public:
  Tfzendnn() {
    // arbitrarily set to 64
    const int inter_op = 64;

    auto model = amdinfer::getPathToAsset("tf_resnet50");

    put("model", model);
    put("input_node", std::string{"input"});
    put("output_node", std::string{"resnet_v1_50/predictions/Reshape_1"});
    put("input_size", kImageHeight);
    put("output_classes", kOutputClasses);
    put("inter_op", inter_op);
    put("intra_op", 1);
  }

  std::string name() const override { return "tfzendnn"; }
  std::string extension() const override { return "tfzendnn"; }
  void updateConfig([[maybe_unused]] Config& config) override {
    // no update necessary
  }

  void preprocess(const std::string& input_path) override {
    amdinfer::pre_post::ImagePreprocessOptions<float, 3> options;
    options.convert_color = true;
    options.color_code = cv::COLOR_BGR2RGB;
    options.assign = true;

    images_ = amdinfer::pre_post::imagePreprocess({input_path}, options);

    InferenceRequest request;
    request.addInputTensor(images_[0].data(),
                           {kImageChannels, kImageHeight, kImageWidth},
                           amdinfer::DataType::Fp32);
    this->request(std::move(request));
  }

 private:
  std::vector<std::vector<float>> images_;
};
#endif  // AMDINFER_ENABLE_TFZENDNN

#ifdef AMDINFER_ENABLE_VITIS
class Vitis : public Backend {
 public:
  Vitis() {
    auto model = amdinfer::getPathToAsset("u250_resnet50");
    put("model", model);
  }

  std::string name() const override { return "xmodel"; }
  std::string extension() const override { return "vitis"; }
  void updateConfig(Config& config) override {
    const int batch_size = 4;
    config.batchSize(batch_size);
  }

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
    request.addInputTensor(images_[0].data(),
                           {kImageHeight, kImageWidth, kImageChannels},
                           amdinfer::DataType::Int8);
    this->request(std::move(request));
  }

 private:
  std::vector<std::vector<int8_t>> images_;
};
#endif  // AMDINFER_ENABLE_VITIS

#ifdef AMDINFER_ENABLE_MIGRAPHX
class Migraphx : public Backend {
 public:
  Migraphx() {
    auto model = amdinfer::getPathToAsset("onnx_resnet50");
    put("model", model);
  }

  std::string name() const override { return "migraphx"; }
  std::string extension() const override { return "migraphx"; }
  void updateConfig([[maybe_unused]] Config& config) override {
    // no update necessary
  }

  void preprocess(const std::string& input_path) override {
    const std::array<float, 3> mean{0.485F, 0.456F, 0.406F};
    const std::array<float, 3> std{4.367F, 4.464F, 4.444F};
    const auto image_size = 224;
    const auto convert_scale = 1 / 255.0;

    amdinfer::pre_post::ImagePreprocessOptions<float, kImageChannels> options;
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
    request.addInputTensor(images_[0].data(),
                           {kImageHeight, kImageWidth, kImageChannels},
                           amdinfer::DataType::Fp32);
    this->request(std::move(request));
  }

 private:
  std::vector<std::vector<float>> images_;
};
#endif  // AMDINFER_ENABLE_MIGRAPHX

enum class Backends {
#ifdef AMDINFER_ENABLE_TFZENDNN
  Tfzendnn,
#endif
#ifdef AMDINFER_ENABLE_PTZENDNN
  Ptzendnn,
#endif
#ifdef AMDINFER_ENABLE_VITIS
  Vitis,
#endif
#ifdef AMDINFER_ENABLE_MIGRAPHX
  Migraphx,
#endif
  // this is used find the number of backends enabled. It is NOT a backend. This
  // must be the last enum value
  Count
};
const auto kNumBackends = static_cast<int>(Backends::Count);

std::unique_ptr<Backend> getBackend(Backends index) {
  switch (index) {
#ifdef AMDINFER_ENABLE_TFZENDNN
    case Backends::Tfzendnn:
      return std::make_unique<Tfzendnn>();
#endif
#ifdef AMDINFER_ENABLE_PTZENDNN
    case Backends::Ptzendnn:
      return std::make_unique<Ptzendnn>();
#endif
#ifdef AMDINFER_ENABLE_VITIS
    case Backends::Vitis:
      return std::make_unique<Vitis>();
#endif
#ifdef AMDINFER_ENABLE_MIGRAPHX
    case Backends::Migraphx:
      return std::make_unique<Migraphx>();
#endif
    default:
      throw amdinfer::invalid_argument("Unknown argument");
  }
}

// the benchmark function cannot be part of a namespace

void resnet50(benchmark::State& st, const amdinfer::Client* client,
              Backend* backend) {
  const auto extension = backend->extension();
  if (!amdinfer::serverHasExtension(client, extension)) {
    std::string error = extension + " support required but not found";
    st.SkipWithError(error.c_str());
    return;
  }

  const auto batch_size = static_cast<int>(st.range(0));
  const auto num_requests = static_cast<int>(st.range(1));
  const auto workers = static_cast<int>(st.range(2));
  Config config{batch_size, num_requests, workers};
  backend->updateConfig(config);
  st.SetLabel(config.toString());

  backend->put("batch_size", config.batchSize());
  backend->put("share", false);

  const auto& request = backend->request();
  const auto& parameters = backend->parameters();

  const auto& name = backend->name();
  auto endpoint = client->workerLoad(name, parameters);
  assert(endpoint == name);
  for (auto i = 0; i < workers - 1; ++i) {
    std::ignore = client->workerLoad(name, parameters);
  }

  const auto warmup_requests = 4;
  for (auto i = 0; i < warmup_requests; ++i) {
    auto response = client->modelInfer(endpoint, request);
    if (response.isError()) {
      st.SkipWithError("Error response from the server");
      return;
    }
  }

  const std::vector<amdinfer::InferenceRequest> requests{
    static_cast<size_t>(config.requests()), request};

  for (auto _ : st) {
    std::ignore = amdinfer::inferAsyncOrdered(client, endpoint, requests);
  }
  for (auto i = 0; i < workers; ++i) {
    client->workerUnload(endpoint);
  }
  amdinfer::waitUntilModelNotReady(client, endpoint);
}

// NOLINTNEXTLINE(cert-err58-cpp)
const std::initializer_list<std::vector<int64_t>> kRange{
  {1, 4, 64},                         // batch size
  benchmark::CreateRange(1, 256, 4),  // number of requests
  benchmark::CreateRange(1, 8, 2)     // workers
};

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

  amdinfer::waitUntilServerReady(client.get());

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
}
