// Copyright 2022 Xilinx, Inc.
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

#ifndef GUARD_AMDINFER_PRE_POST_IMAGE_PREPROCESS
#define GUARD_AMDINFER_PRE_POST_IMAGE_PREPROCESS

#include <array>
#include <cassert>
#include <cstdint>
#include <opencv2/core.hpp>       // for Mat, Vec3b, MatSize, Vec, CV_8SC3
#include <opencv2/imgcodecs.hpp>  // for imread
#include <opencv2/imgproc.hpp>    // for resize
#include <stdexcept>
#include <string>
#include <vector>

#include "amdinfer/pre_post/center_crop.hpp"

namespace amdinfer::pre_post {

enum class ImageOrder {
  NHWC,
  NCHW,
};

enum class ResizeAlgorithm {
  Simple,
  CenterCrop,
  LetterBoxCrop,
};

const auto kDefaultImageSize = 224;

template <typename T, int kChannels>
struct ImagePreprocessOptions {
  int height = kDefaultImageSize;
  int width = kDefaultImageSize;
  int channels = kChannels;

  bool resize = true;
  ResizeAlgorithm resize_algorithm = ResizeAlgorithm::Simple;

  bool convert_color = false;
  // this should be cv::ColorConversionCodes but we can't bind it to Python
  // conveniently
  int color_code = 0;

  bool convert_type = false;
  int type = 0;
  double convert_scale = 0;

  bool normalize = false;
  ImageOrder order = ImageOrder::NHWC;
  std::array<T, kChannels> mean;
  std::array<T, kChannels> std;

  bool assign = false;
};

namespace detail {

template <typename T, typename F>
void nestedLoop(int a, int b, int c, T* output, F f) {
  for (int i = 0; i < a; i++) {
    for (int j = 0; j < b; j++) {
      for (int k = 0; k < c; k++) {
        auto output_index = (i * b * c) + (j * c) + k;
        output[output_index] = static_cast<T>(f(i, j, k));
      }
    }
  }
}

template <typename T, int kChannels>
void normalize(const cv::Mat& img, ImageOrder order, T* output, const T* mean,
               const T* std) {
  auto height = img.size[0];
  auto width = img.size[1];
  switch (order) {
    case ImageOrder::NHWC:
      nestedLoop(
        height, width, kChannels, output, [&](int h, int w, int c) -> T {
          return (static_cast<T>(img.at<cv::Vec<T, kChannels>>(h, w)[c]) -
                  mean[c]) *
                 std[c];
        });
      break;
    case ImageOrder::NCHW:
      nestedLoop(kChannels, height, width, output, [&](int c, int h, int w) {
        return (static_cast<T>(img.at<cv::Vec<T, kChannels>>(h, w)[c]) -
                mean[c]) *
               std[c];
      });
      break;
    default:
      throw std::invalid_argument("Unknown image order");
  }
}

}  // namespace detail

template <typename T, int kChannels = 3>
std::vector<std::vector<T>> imagePreprocess(
  const std::vector<std::string>& paths,
  const ImagePreprocessOptions<T, 3>& options) {
  std::vector<std::vector<T>> outputs;
  outputs.reserve(paths.size());

  const auto& height = options.height;
  const auto& width = options.width;
  const auto& channels = options.channels;

  assert(channels == kChannels);

  auto index = 0;
  for (const auto& path : paths) {
    auto img = cv::imread(path);
    if (img.empty()) {
      throw std::invalid_argument(std::string("Unable to load image ") + path);
    }
    if (options.convert_color) {
      cv::cvtColor(img, img, options.color_code);
    }

    if (options.resize) {
      switch (options.resize_algorithm) {
        case ResizeAlgorithm::Simple:
          cv::resize(img, img, cv::Size(width, height), cv::INTER_LINEAR);
          break;
        case ResizeAlgorithm::CenterCrop:
          img = centerCrop(img, height, width);
          break;
        default:
          throw std::invalid_argument("Unknown resize algorithm");
      }
    }

    if (options.convert_type) {
      img.convertTo(img, options.type, options.convert_scale);
    }
    img = img.isContinuous() ? img : img.clone();

    auto size = img.size[0] * img.size[1] * channels;
    outputs.emplace_back();
    auto& output = outputs[index++];
    output.resize(size);

    if (options.normalize) {
      const auto* mean = options.mean.data();
      const auto* std = options.std.data();
      detail::normalize<T, kChannels>(img, options.order, output.data(), mean,
                                      std);
    }

    if (options.assign) {
      output.assign(img.data, img.data + size);
    }
  }
  return outputs;
}

}  // namespace amdinfer::pre_post

#endif  // GUARD_AMDINFER_PRE_POST_IMAGE_PREPROCESS
