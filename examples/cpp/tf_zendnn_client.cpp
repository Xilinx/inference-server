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
 * @brief Implements a client application for TfZendnn worker
 */

#include <algorithm>              // for copy, max
#include <chrono>                 // for milliseconds, duration_cast, operator-
#include <cmath>                  // for exp
#include <cstdint>                // for uint64_t
#include <cstdlib>                // for exit, getenv, size_t
#include <fstream>                // IWYU pragma: keep
#include <initializer_list>       // for initializer_list
#include <iostream>               // for operator<<, basic_ostream, endl
#include <memory>                 // for allocator_traits<>::value_type
#include <opencv2/core.hpp>       // for Mat, MatSize, operator*, operator-
#include <opencv2/imgcodecs.hpp>  // for imread
#include <opencv2/imgproc.hpp>    // for resize, cvtColor, INTER_LINEAR, COL...
#include <queue>                  // for priority_queue, queue
#include <string>                 // for string, operator+, allocator, opera...
#include <unordered_set>          // for operator==, unordered_set, unordere...
#include <utility>                // for pair
#include <vector>                 // for vector

#include "proteus/proteus.hpp"  // for InferenceResponseFuture, terminate
#include "proteus/util/pre_post/image_preprocess.hpp"
#include "proteus/util/pre_post/resnet50_postprocess.hpp"
#include "proteus/util/read_nth_line.hpp"

/**
 * @brief user variables: update as needed!
 * If image location is provided, the prediction is done for that image,
 * If not provided, dummy data will be created for functionality testing.
 *
 */
struct Option {
  std::string root = std::getenv("PROTEUS_ROOT");

  std::string graph =
    root + "/external/tensorflow_models/resnet_v1_50_baseline_6.96B_922.pb";
  std::string image_location = root + "/tests/assets/dog-3619020_640.jpg";
  // std::string image_location = "";
  std::string input_node = "input";
  std::string output_node = "resnet_v1_50/predictions/Reshape_1";
  std::string resize_method = "crop";
  std::string class_file = root + "/examples/python/utils/imagenet_classes.txt";

  int batch_size = 64;
  int input_size = 224;
  int inter_op = 64;
  int intra_op = 1;
  int warmup_step = 5;
  int steps = 1;
  int topK = 5;
} options;

/**
 * @brief The example demonstrates how to plug in custom pre-
 * and post-processing around a call to Proteus. Pre- and
 * post-processing is done in C++ before and after the call to Proteus.
 *
 * @return int
 */
int main() {
  auto* root_env = std::getenv("PROTEUS_ROOT");
  if (root_env == nullptr) {
    std::cerr << "PROTEUS_ROOT is not defined in the environment\n";
    return 1;
  }
  std::string root = root_env;

  // initialize the server
  proteus::Server server;

  auto client = proteus::NativeClient();
  auto metadata = client.serverMetadata();
  if (metadata.extensions.find("tfzendnn") == metadata.extensions.end()) {
    std::cout << "TFZenDNN support required but not found.\n";
    exit(0);
  }

  // load worker with required parameters
  proteus::RequestParameters parameters;
  // parameters.put("max_buffer_num", options.batch_size);
  parameters.put("model", options.graph);
  parameters.put("input_node", options.input_node);
  parameters.put("output_node", options.output_node);
  parameters.put("input_size", options.input_size);
  parameters.put("inter_op", options.inter_op);
  parameters.put("intra_op", options.intra_op);
  parameters.put("batch_size", options.batch_size);
  auto workerName = client.workerLoad("TfZendnn", &parameters);

  float time_tmp = 0.f;
  // prepare images for inference
  if (!options.image_location.empty()) {
    std::vector<std::string> paths;
    paths.emplace_back(options.image_location);
    proteus::util::ImagePreprocessOptions<float, 3> options_;
    options_.convert_color = true;
    options_.color_code = cv::COLOR_BGR2RGB;
    options_.assign = true;
    auto images = proteus::util::imagePreprocess(paths, options_);

    const std::initializer_list<uint64_t> shape = {
      static_cast<uint64_t>(options.input_size),
      static_cast<uint64_t>(options.input_size), 3};
    std::queue<proteus::InferenceResponseFuture> queue;
    proteus::InferenceRequest request;
    auto start = std::chrono::high_resolution_clock::now();  // Timing the start
    request.addInputTensor(static_cast<void*>(images[0].data()), shape,
                           proteus::DataType::FP32);
    auto results = client.modelInfer(workerName, request);
    // Timing the prediction
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    time_tmp = duration.count();

    auto outputs = results.getOutputs();
    for (auto& output : outputs) {
      auto top_k = proteus::util::resnet50Postprocess(
        static_cast<float*>(output.getData()), output.getSize(), options.topK);
      std::cout << "Top " << options.topK << " classes:" << std::endl;
      for (int j = 0; j < options.topK; j++)
        std::cout << j + 1 << ": Index: " << top_k[j] << " :: Class: "
                  << proteus::util::readNthLine(options.class_file, top_k[j])
                  << std::endl;
    }
  } else {
    std::vector<std::vector<float>> images;
    images.reserve(options.batch_size);

    auto size = options.input_size * options.input_size * 3;
    for (auto i = 0; i < options.batch_size; i++) {
      cv::Mat img = cv::Mat(options.input_size, options.input_size, CV_32FC3);
      cv::randu(img, cv::Scalar::all(0), cv::Scalar::all(1));

      images.emplace_back();
      auto& image = images[i];
      image.resize(size);
      float* arr =
        img.isContinuous() ? img.ptr<float>(0) : img.clone().ptr<float>(0);
      image.assign(arr, arr + size);
    }

    // inference
    const std::initializer_list<uint64_t> shape = {
      static_cast<uint64_t>(options.input_size),
      static_cast<uint64_t>(options.input_size), 3};
    // Warmup laps to get the best performance
    for (int step = 0; step < options.warmup_step; step++) {
      std::queue<proteus::InferenceResponseFuture> queue;
      for (auto i = 0; i < options.batch_size; i++) {
        proteus::InferenceRequest request;
        request.addInputTensor(static_cast<void*>(images[i].data()), shape,
                               proteus::DataType::FP32);
        auto results = client.modelInfer(workerName, request);
      }
    }

    // Running for `steps` number of time for proper benchmarking
    auto start = std::chrono::high_resolution_clock::now();  // Timing the start
    for (int step = 0; step < options.steps; step++) {
      std::queue<proteus::InferenceResponseFuture> queue;
      for (auto i = 0; i < options.batch_size; i++) {
        proteus::InferenceRequest request;
        request.addInputTensor(static_cast<void*>(images[i].data()), shape,
                               proteus::DataType::FP32);
        auto results = client.modelInfer(workerName, request);
      }
    }
    // Timing the prediction
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    time_tmp = duration.count() / options.steps;
  }

  if (!options.image_location.empty()) {
    std::cout << "\nTime taken for " + options.image_location + " : " +
                   std::to_string(time_tmp)
              << "ms" << std::endl;
  } else {
    std::cout << "\nAverage time taken for " +
                   std::to_string(options.batch_size) +
                   " images: " + std::to_string(time_tmp)
              << std::endl;
    std::cout << "Batch Size: " + std::to_string(options.batch_size) +
                   " FPS: " +
                   std::to_string(options.batch_size / time_tmp * 1000)
              << std::endl;
  }

  return 0;
}
