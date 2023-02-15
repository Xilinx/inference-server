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

#include <mlcommons/loadgen/loadgen.h>
#include <mlcommons/loadgen/test_settings.h>

#include <cxxopts/cxxopts.hpp>
#include <filesystem>
#include <opencv2/core.hpp>       // for Mat, Vec3b, MatSize, Vec, CV_8SC3
#include <opencv2/imgcodecs.hpp>  // for imread
#include <opencv2/imgproc.hpp>    // for resize

#include "query_sample_library.hpp"
#include "system_under_test.hpp"

namespace fs = std::filesystem;

amdinfer::InferenceRequest pre_process_resnet50(
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
          (img.at<cv::Vec<int8_t, channels>>(i, j)[k] - mean[k]) * std[k]);
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
  size_t performance_samples = 1000;
  fs::path input_directory = fs::current_path() / "data";
  fs::path model_path;
  std::string worker;

  try {
    cxxopts::Options options(
      "mlperf", "Run the mlperf benchmark with the AMD Inference Server");
    // clang-format off
    options.add_options()
    ("performance-samples",
      "Number of samples guaranteed to fit in memory. Defaults to 1000.",
      cxxopts::value(performance_samples))
    ("input-directory",
      "Path to the directory containing input data. Defaults to ./data",
      cxxopts::value(input_directory))
    ("model", "Path to the model file", cxxopts::value(model_path))
    ("worker", "Name of the worker to use", cxxopts::value(worker))
    ("help", "Print help");
    // clang-format on

    auto result = options.parse(argc, argv);

    if (result.count("help") != 0U) {
      std::cout << options.help({""}) << "\n";
      exit(0);
    }
  } catch (const cxxopts::OptionException& e) {
    std::cout << "Error parsing options: " << e.what() << "\n";
    exit(1);
  }

  if (!fs::exists(input_directory)) {
    std::cerr << "Input directory at " << input_directory
              << " does not exist\n";
    return 1;
  }

  if (!fs::exists(model_path)) {
    std::cerr << "Model at " << model_path << " does not exist\n";
    return 1;
  }

  if (worker.empty()) {
    std::cerr << "A valid worker must be specified with --worker\n";
    return 1;
  }

  mlperf::TestSettings testSettings;
  testSettings.scenario = mlperf::TestScenario::SingleStream;
  testSettings.mode = mlperf::TestMode::PerformanceOnly;
  testSettings.min_query_count = 10000;
  // testSettings.max_query_count = 10000;
  testSettings.min_duration_ms =
    10;  // TO-DO: expose it to the outer level user command
  testSettings.server_max_async_queries = 1;
  // testSettings.multi_stream_target_qps = 20;
  testSettings.multi_stream_samples_per_query = 52;
  testSettings.server_target_latency_ns = 15000000;
  testSettings.server_target_qps = 100;
  testSettings.offline_expected_qps = 100;
  // testSettings.qsl_rng_seed = 12786827339337101903ULL;
  // testSettings.schedule_rng_seed = 3135815929913719677ULL;
  // testSettings.sample_index_rng_seed = 12640797754436136668ULL;
  testSettings.qsl_rng_seed = 7322528924094909334ULL;
  testSettings.schedule_rng_seed = 3507442325620259414ULL;
  testSettings.sample_index_rng_seed = 1570999273408051088ULL;

  mlperf::LogSettings logSettings;
  logSettings.enable_trace = false;

  amdinfer::QuerySampleLibrary qsl(performance_samples, input_directory,
                                   pre_process_resnet50);

  amdinfer::ParameterMap parameters;
  parameters.put("model", model_path);
  parameters.put("batch_size", 4);
  amdinfer::SystemUnderTestNative sut(&qsl, worker, parameters);

  mlperf::StartTest(&sut, &qsl, testSettings, logSettings);

  return 0;
}
