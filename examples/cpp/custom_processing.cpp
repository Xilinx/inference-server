// Copyright 2021 Xilinx Inc.
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

// +include:
#include "proteus/proteus.hpp"  // for InferenceResponseFuture, terminate
// -include:

/**
 * @brief Perform preprocessing for the input images
 *
 * @param paths paths to images to preprocess
 * @return std::vector<std::vector<int8_t>> preprocessed image data
 */
std::vector<std::vector<int8_t>> preprocess(
  const std::vector<std::string>& paths) {
  // update as needed!
  const auto height = 224;
  const auto width = 224;
  const auto channels = 3;
  std::string layout = "NHWC";
  const auto fix_scale = 1;
  const std::array mean = {123, 107, 104};

  std::vector<std::vector<int8_t>> outputs;
  outputs.reserve(paths.size());

  auto index = 0;
  for (const auto& path : paths) {
    auto img = cv::imread(path);

    cv::Mat resizedImg = cv::Mat(height, width, CV_8SC3);
    cv::resize(img, resizedImg, cv::Size(width, height));
    auto size = resizedImg.size[0] * resizedImg.size[1] * channels;
    outputs.emplace_back();
    auto& output = outputs[index];
    output.resize(size);

    if (layout == "NCHW") {
      for (int c = 0; c < channels; c++) {
        for (int h = 0; h < height; h++) {
          for (int w = 0; w < width; w++) {
            output[(c * height * width) + (h * width) + w] =
              (resizedImg.at<cv::Vec3b>(h, w)[c] - mean[c]) * fix_scale;
          }
        }
      }
    } else if (layout == "NHWC") {
      for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
          for (int c = 0; c < channels; c++) {
            output[h * width * channels + w * channels + c] =
              (resizedImg.at<cv::Vec3b>(h, w)[c] - mean[c]) * fix_scale;
          }
        }
      }
    }
    index++;
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
void calc_softmax(const int8_t* data, size_t size, double* result) {
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
  auto* data = static_cast<std::vector<int8_t>*>(output.getData());
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
int main() {
  auto* root_env = std::getenv("PROTEUS_ROOT");
  if (root_env == nullptr) {
    std::cerr << "PROTEUS_ROOT is not defined in the environment\n";
    return 1;
  }
  std::string root = root_env;

  // +user variables: update as needed!
  const auto batch_size = 4;
  const auto* path_to_xmodel =
    "${AKS_XMODEL_ROOT}/artifacts/u200_u250/resnet_v1_50_tf/"
    "resnet_v1_50_tf.xmodel";
  const auto path_to_image = root + "/tests/assets/dog-3619020_640.jpg";
  // for this image, we know what we expect to receive with this XModel
  const std::array gold_response_output = {259, 261, 260, 230, 154};
  const auto k = gold_response_output.size();
  // -user variables:

  // +initialize:
  proteus::initialize();
  // -initialize:

  // +load:
  proteus::RequestParameters parameters;
  parameters.put("xmodel", path_to_xmodel);
  auto workerName = proteus::load("Xmodel", &parameters);
  // -load:

  // +prepare images:
  std::vector<std::string> paths;
  paths.reserve(batch_size);

  for (auto i = 0; i < batch_size; i++) {
    paths.emplace_back(path_to_image);
  }

  auto images = preprocess(paths);
  // -prepare images:

  // +inference:
  const std::initializer_list<uint64_t> shape = {224, 224, 3};
  std::queue<proteus::InferenceResponseFuture> queue;

  for (auto i = 0; i < batch_size; i++) {
    proteus::InferenceRequestInput request(static_cast<void*>(images[i].data()),
                                           shape,
                                           proteus::types::DataType::INT8);
    queue.push(proteus::enqueue(workerName, request));
  }
  // -inference:

  for (auto i = 0; i < batch_size; i++) {
    // +get output:
    auto front = std::move(queue.front());
    queue.pop();
    auto results = front.get();
    // -get output:

    // +validate:
    auto outputs = results.getOutputs();
    for (auto& output : outputs) {
      auto top_k = postprocess(output, k);
      for (size_t j = 0; j < k; j++) {
        if (top_k[j] != gold_response_output[j]) {
          std::cerr << "Output (" << top_k[j] << ") does not match golden ("
                    << gold_response_output[j] << ")\n";
          proteus::terminate();
          return 1;
        }
      }
    }
    // -validate:
  }
  // -validate:

  // +clean:
  proteus::terminate();
  // -clean:

  std::cout << "custom_processing.cpp: Passed\n";

  return 0;
}
