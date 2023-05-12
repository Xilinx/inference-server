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

/**
 * @file
 * @brief
 */

#include <loadgen.h>
#include <test_settings.h>

#include <cxxopts.hpp>
#include <filesystem>
#include <opencv2/core.hpp>       // for Mat, Vec3b, MatSize, Vec, CV_8SC3
#include <opencv2/imgcodecs.hpp>  // for imread
#include <opencv2/imgproc.hpp>    // for resize

#include "config_parser.hpp"
#include "query_sample_library.hpp"
#include "system_under_test.hpp"

namespace fs = std::filesystem;

amdinfer::InferenceRequest preprocessResnet50(
  const std::filesystem::path& path) {
  const auto height = 224;
  const auto width = 224;
  const auto channels = 3;
  const std::array<int8_t, 3> mean{123, 107, 104};
  const std::array<int8_t, 3> std{1, 1, 1};

  auto img = cv::imread(path.string());
  img = img.isContinuous() ? img : img.clone();
  auto size = img.size[0] * img.size[1] * channels;

  std::vector<std::byte> data;
  data.resize(size * sizeof(int8_t));

  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      for (int k = 0; k < channels; k++) {
        auto output_index = (i * width * channels) + (j * channels) + k;
        auto* addr = reinterpret_cast<int8_t*>(data.data() + output_index);
        *addr = static_cast<int8_t>(
          (img.at<cv::Vec<int8_t, channels>>(i, j)[k] - mean.at(k)) *
          std.at(k));
      }
    }
  }

  amdinfer::InferenceRequest request;
  amdinfer::InferenceRequestInput tensor;
  tensor.setShape({height, width, channels});
  tensor.setData(std::move(data));
  tensor.setDatatype(amdinfer::DataType::Int8);

  request.addInputTensor(tensor);
  return request;
}

int main(int argc, char* argv[]) {
  const size_t default_performance_samples = 1000;

  // these defaults are overridden first by the config file and then by command
  // line, if they exist
  size_t performance_samples = default_performance_samples;
  fs::path input_directory = fs::current_path() / "data";
  fs::path model_path;
  std::string worker;
  std::string client_id{"native"};
  std::string address;
  std::string endpoint;
  bool remote_server = false;

  // these must be specified from the command line
  std::string scenario;
  std::string model;
  fs::path config_path = fs::current_path() / "mlperf.conf";
  fs::path mlperf_config_path = fs::current_path() / "mlperf_filtered.conf";

  mlperf::TestSettings test_settings;
  amdinfer::Config test_config;

  cxxopts::Options options(
    "mlperf", "Run the mlperf benchmark with the AMD Inference Server");
  // clang-format off
  options.add_options()
  ("config", "Path to the config file. Defaults to ./mlperf.conf",
    cxxopts::value(config_path))
  ("model", "Must be one of the mlperf model names", cxxopts::value(model))
  ("scenario",
    "Must be one of 'SingleStream', 'MultiStream', 'Server', 'Offline'",
    cxxopts::value(scenario))
  ("performance-samples",
    "Number of samples guaranteed to fit in memory. Defaults to 1000.",
    cxxopts::value(performance_samples))
  ("input-directory",
    "Path to the directory containing input data. Defaults to ./data",
    cxxopts::value(input_directory))
  // ("client", "Must be one of 'native', 'HTTP' or 'gRPC'",
  //   cxxopts::value(client_id))
  // ("address", "Address to the server if using HTTP or gRPC client",
  //   cxxopts::value(address))
  // ("remote-server", "Set to use remote server",
  //   cxxopts::value(remote_server))
  // ("endpoint", "The endpoint for inference if using a remote server",
  //   cxxopts::value(endpoint))
  ("model-path", "Path to the model file if using a local server",
    cxxopts::value(model_path))
  ("worker", "Name of the worker to use if using a local server",
    cxxopts::value(worker))
  ("help", "Print help");
  // clang-format on

  try {
    auto result = options.parse(argc, argv);

    if (result.count("help") != 0U) {
      std::cout << options.help({""}) << "\n";
      return 0;
    }

    if (!fs::exists(config_path)) {
      std::cerr << "Config file at " << config_path << " does not exist\n";
      return 1;
    }

    if (scenario.empty()) {
      std::cerr << "Scenario not specified. Use --scenario <arg>\n";
      return 1;
    }

    if (model.empty()) {
      std::cerr << "Model not specified. Use --model <arg>\n";
      return 1;
    }

    mlperf_config_path = config_path.parent_path() / "mlperf_filtered.conf";
    test_config = amdinfer::parseConfig(config_path, mlperf_config_path);

    // if these arguments aren't specified on the command-line, override them
    // with values from the config file, if they exist
    if (result.count("performance-samples") == 0U &&
        test_config.has(model, scenario, "performance_samples")) {
      performance_samples =
        test_config.get<int>(model, scenario, "performance_samples");
    }
    if (result.count("worker") == 0U &&
        test_config.has(model, scenario, "worker")) {
      worker = test_config.get<std::string>(model, scenario, "worker");
    }
    if (result.count("input-directory") == 0U &&
        test_config.has(model, scenario, "input_directory")) {
      input_directory =
        test_config.get<std::string>(model, scenario, "input_directory");
    }

  } catch (const cxxopts::OptionException& e) {
    std::cout << "Error parsing options: " << e.what() << "\n";
    exit(1);
  }

  if (test_config.has(model, scenario, "client_id")) {
    client_id = test_config.get<std::string>(model, scenario, "client_id");
  }

  if (test_config.has(model, scenario, "address")) {
    address = test_config.get<std::string>(model, scenario, "address");
  }

  if (test_config.has(model, scenario, "remote_server")) {
    remote_server = test_config.get<bool>(model, scenario, "remote_server");
  }

  if (test_config.has(model, scenario, "endpoint")) {
    endpoint = test_config.get<std::string>(model, scenario, "endpoint");
  }

  if (scenario == "SingleStream") {
    test_settings.scenario = mlperf::TestScenario::SingleStream;
  } else if (scenario == "MultiStream") {
    test_settings.scenario = mlperf::TestScenario::MultiStream;
  } else if (scenario == "Server") {
    test_settings.scenario = mlperf::TestScenario::Server;
  } else if (scenario == "Offline") {
    test_settings.scenario = mlperf::TestScenario::Offline;
  } else {
    std::cerr << "Scenario must be one of 'SingleStream', 'MultiStream', "
                 "'Server', or 'Offline'\n";
    return 1;
  }
  auto retval =
    test_settings.FromConfig(mlperf_config_path.string(), model, scenario);
  if (retval != 0) {
    std::cerr << "Errors encountered during parsing configuration file\n";
    return retval;
  }

  mlperf::LogSettings log_settings;
  log_settings.enable_trace = false;

  if (!fs::exists(input_directory)) {
    std::cerr << "Input directory at " << input_directory
              << " does not exist\n";
    return 1;
  }

  if (worker.empty()) {
    std::cerr << "A valid worker must be specified with --worker or in the "
                 "config file\n";
    return 1;
  }

  amdinfer::QuerySampleLibrary qsl(performance_samples, input_directory,
                                   preprocessResnet50);

  std::optional<amdinfer::Server> server;
  std::unique_ptr<amdinfer::Client> client;
  if (!remote_server) {
    server.emplace();
  }

  // TODO(varunsh): expose to the outside
  [[maybe_unused]] const uint16_t http_port = 8998;
  [[maybe_unused]] const uint16_t grpc_port = 50'051;

  if (client_id == "native") {
    if (remote_server) {
      std::cerr << "Server must be started locally if using native client\n";
      return 1;
    }
    client = std::make_unique<amdinfer::NativeClient>(&(server.value()));
#ifdef AMDINFER_ENABLE_HTTP
  } else if (client_id == "HTTP") {
    client = std::make_unique<amdinfer::HttpClient>(address);
    if (!remote_server) {
      server.value().startHttp(http_port);
    }
#endif
#ifdef AMDINFER_ENABLE_GRPC
  } else if (client_id == "gRPC") {
    client = std::make_unique<amdinfer::GrpcClient>(address);
    if (!remote_server) {
      server.value().startGrpc(grpc_port);
    }
#endif
  } else {
    std::cerr << "Client must be one of 'native', 'HTTP', or 'gRPC'\n";
    return 1;
  }

  if (endpoint.empty() || endpoint == "n/a") {
    amdinfer::ParameterMap parameters =
      test_config.getParameters(model, scenario);
    parameters.put("share", false);
    amdinfer::waitUntilServerReady(client.get());

    endpoint = client->workerLoad(worker, &parameters);
    if (test_config.has(model, scenario, "workers")) {
      auto workers = test_config.get<int>(model, scenario, "workers");
      for (auto i = 0; i < workers - 1; ++i) {
        client->workerLoad(worker, &parameters);
      }
    }
  }

  amdinfer::SystemUnderTest sut(&qsl, client.get(), endpoint);

  mlperf::StartTest(&sut, &qsl, test_settings, log_settings);

  return 0;
}
