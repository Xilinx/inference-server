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

#include <array>                  // for array
#include <cmath>                  // for exp
#include <cstdint>                // for int8_t, uint64_t
#include <cstdlib>                // for getenv, size_t
#include <fstream>                // For files
#include <future>                 // for future
#include <initializer_list>       // for initializer_list
#include <iostream>               // for operator<<, basic_ostream::operator<<
#include <opencv2/core.hpp>       // for Mat, Vec3b, MatSize, Vec, CV_8SC3
#include <opencv2/imgcodecs.hpp>  // for imread
#include <opencv2/imgproc.hpp>    // for resize
#include <queue>                  // for priority_queue, queue
#include <string>                 // for string, allocator, operator==, basi...
#include <utility>                // for pair, move
#include <vector>                 // for vector

#include "proteus/proteus.hpp"  // for InferenceResponseFuture, terminate

/**
 * @brief This method will read the class file and returns class name
 *
 * @param filename: The location of the file containing class names
 * @param N: Nth value from the class file
 * @return std::string: Name of the class
 */
std::string ReadNthLine(const std::string filename, int N) {
  std::ifstream in(filename);
  std::string line;
  line.reserve(100);  // for performance
  // skip N lines
  for (int i = 0; i < N; ++i) std::getline(in, line);

  std::getline(in, line);
  return line;
}

/**
 * @brief Crops the input `img` from center to a specific dimension specified
 *
 * @param img: Image which needs to be cropped
 * @param height: height of the output image
 * @param width: width of the output image
 * @return cv::Mat: Image cropped to required shape
 */
cv::Mat center_crop(cv::Mat img, int height, int width) {
  const int offsetW = (img.cols - width) / 2;
  const int offsetH = (img.rows - height) / 2;
  const cv::Rect roi(offsetW, offsetH, width, height);
  img = img(roi);
  return img;
}

/**
 * @brief This image will load and preprocess the data according to
 * the input arguments
 *
 * @param paths: The location of the image to be loaded
 * @param img_size: The input height/width of the image
 * @return std::vector<std::vector<float>>: The preprocessed image
 */
std::vector<std::vector<float>> preprocess(
  const std::vector<std::string>& paths, int img_size,
  std::string method = "default") {
  // update as needed!

  std::vector<std::vector<float>> outputs;
  outputs.reserve(paths.size());

  auto index = 0;
  for (const auto& path : paths) {
    auto img = cv::imread(path);
    cv::cvtColor(img, img, cv::COLOR_BGR2RGB);

    outputs.emplace_back();
    auto& output = outputs[index++];

    if (!method.compare("default")) {
      // Preprocess
      auto new_size = cv::Size(256, 256);
      cv::resize(img, img, new_size, cv::INTER_LINEAR);
      img = center_crop(img, img_size, img_size);
      img.convertTo(img, CV_32FC3, 1.0 / 255.0);
      img = img.isContinuous() ? img : img.clone();

      const std::array mean = {0.485, 0.456, 0.406};
      const std::array std = {0.229, 0.224, 0.225};

      // Copy to the vector
      auto size = img.size[0] * img.size[1] * 3;
      output.resize(size);
      for (int c = 0; c < 3; c++) {
        for (int h = 0; h < img_size; h++) {
          for (int w = 0; w < img_size; w++) {
            output[(c * img_size * img_size) + (h * img_size) + w] =
              (img.at<cv::Vec3f>(h, w)[c] - mean.at(c)) / std.at(c);
          }
        }
      }
    } else {
      std::cerr << "Preprocess algo " << method << " not implemented";
      exit(0);
    }
  }
  return outputs;
}

/**
 * @brief Calculate softmax of the data
 *
 * @param data pointer to the raw data
 * @param size number of elements in the raw data
 * @param result pointer to store the computed results
 */
void calc_softmax(const float* data, size_t size, double* result) {
  double sum = 0;

  auto max = data[0];
  for (size_t i = 1; i < size; i++) {
    if (data[i] > max) {
      max = data[i];
    }
  }

  for (size_t i = 0; i < size; i++) {
    result[i] = exp(data[i] - max);
    sum += result[i];
  }

  for (size_t i = 0; i < size; i++) {
    result[i] /= sum;
  }
}

/**
 * @brief After running softmax, get the labels associated with the top k values
 *
 * @param d pointer to the data
 * @param size number of elements in the data
 * @param k number of top elements to return
 * @return std::vector<int>
 */
std::vector<int> get_top_k(const double* d, int size, int k) {
  std::priority_queue<std::pair<float, int>> q;

  for (auto i = 0; i < size; ++i) {
    q.push(std::pair<float, int>(d[i], i));
  }
  std::vector<int> topKIndex;
  for (auto i = 0; i < k; ++i) {
    std::pair<float, int> ki = q.top();
    q.pop();
    topKIndex.push_back(ki.second);
  }
  return topKIndex;
}

/**
 * @brief Perform postprocessing of the data
 *
 * @param output output from Proteus
 * @param k number of top elements to return
 * @return std::vector<int>
 */
std::vector<int> postprocess(proteus::InferenceResponseOutput& output, int k) {
  auto* data = static_cast<std::vector<float>*>(output.getData());
  auto size = output.getSize();

  std::vector<double> softmax;
  softmax.resize(size);

  calc_softmax(data->data(), size, softmax.data());
  return get_top_k(softmax.data(), size, k);
}

/**
 * @brief The custom processing example demonstrates how to plug in custom pre-
 * and post-processing around a call to Proteus. This example uses Resnet50, a
 * classification model, in the Xmodel worker in Proteus. Pre- and
 * post-processing is done in C++ before and after the call to Proteus.
 *
 * @return int
 */

/**
 * @brief user variables: update as needed!
 * If image location is provided, the prediction is done for that image,
 * If not provided, dummy data will be created for functionality testing.
 *
 */
struct Option {
  std::string root = std::getenv("PROTEUS_ROOT");

  std::string graph = root + "/external/pytorch_models/resnet50_pretrained.pt";
  std::string image_location = root + "/tests/assets/dog-3619020_640.jpg";
  // std::string image_location = "";
  std::string resize_method = "default";
  std::string class_file = root + "/examples/python/utils/imagenet_classes.txt";

  int batch_size = 640;
  int input_size = 224;
  int warmup_step = 5;
  int steps = 10;
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
  proteus::initialize();

  auto client = proteus::NativeClient();
  auto metadata = client.serverMetadata();
  if (metadata.extensions.find("ptzendnn") == metadata.extensions.end()) {
    std::cout << "PTZenDNN support required but not found.\n";
    proteus::terminate();
    exit(0);
  }

  // load worker with required parameters
  proteus::RequestParameters parameters;
  parameters.put("max_buffer_num", options.batch_size);
  parameters.put("model", options.graph);
  parameters.put("input_size", options.input_size);
  auto workerName = client.workerLoad("PtZendnn", &parameters);

  float time_tmp = 0.f;
  // prepare images for inference
  if (!options.image_location.empty()) {
    std::vector<std::string> paths;
    paths.emplace_back(options.image_location);
    auto images = preprocess(paths, options.input_size, options.resize_method);

    const std::initializer_list<uint64_t> shape = {
      3, static_cast<long unsigned>(options.input_size),
      static_cast<long unsigned>(options.input_size)};
    std::queue<proteus::InferenceResponseFuture> queue;
    proteus::InferenceRequest request;
    request.addInputTensor(static_cast<void*>(images[0].data()), shape,
                           proteus::types::DataType::FP32);
    // Timing the prediction
    auto start = std::chrono::high_resolution_clock::now();  // Timing the start
    auto results = client.modelInfer(workerName, request);
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    time_tmp = duration.count();

    auto outputs = results.getOutputs();
    for (auto& output : outputs) {
      auto top_k = postprocess(output, options.topK);
      std::cout << "Top " << options.topK << " classes:" << std::endl;
      for (int j = 0; j < options.topK; j++)
        std::cout << j + 1 << ": Index: " << top_k[j]
                  << " :: Class: " << ReadNthLine(options.class_file, top_k[j])
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
      3, static_cast<long unsigned>(options.input_size),
      static_cast<long unsigned>(options.input_size)};
    // Warmup laps to get the best performance
    for (int step = 0; step < options.warmup_step; step++) {
      proteus::InferenceRequest request;
      for (auto i = 0; i < options.batch_size; i++) {
        request.addInputTensor(static_cast<void*>(images[i].data()), shape,
                               proteus::types::DataType::FP32);
      }
      auto results = client.modelInfer(workerName, request);
    }

    // Running for `steps` number of time for proper benchmarking
    auto start = std::chrono::high_resolution_clock::now();  // Timing the start
    for (int step = 0; step < options.steps; step++) {
      std::queue<proteus::InferenceResponseFuture> queue;
      proteus::InferenceRequest request;
      for (auto i = 0; i < options.batch_size; i++) {
        request.addInputTensor(static_cast<void*>(images[i].data()), shape,
                               proteus::types::DataType::FP32);
      }
      auto results = client.modelInfer(workerName, request);
    }
    // Timing the prediction
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    time_tmp = duration.count() / options.steps;
  }

  if (!options.image_location.empty())
    std::cout << "\nTime taken for " + options.image_location + " : " +
                   std::to_string(time_tmp)
              << "ms" << std::endl;
  else {
    std::cout << "\nAverage time taken for " +
                   std::to_string(options.batch_size) +
                   " images: " + std::to_string(time_tmp)
              << std::endl;
    std::cout << "Batch Size: " + std::to_string(options.batch_size) +
                   " FPS: " +
                   std::to_string(options.batch_size / time_tmp * 1000)
              << std::endl;
  }

  // clean up
  proteus::terminate();

  return 0;
}
