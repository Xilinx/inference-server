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

#ifndef GUARD_AMDINFER_PRE_POST_CENTER_CROP
#define GUARD_AMDINFER_PRE_POST_CENTER_CROP

#include <opencv2/core.hpp>  // for cv::Mat

namespace amdinfer::pre_post {

/**
 * @brief Image preprocessing helper.  Crops the input `img` from center to a
 * specific dimension specified
 *
 * @param img: Image which needs to be cropped
 * @param height: height of the output image
 * @param width: width of the output image
 * @return cv::Mat: Image cropped to required shape
 */
inline cv::Mat centerCrop(cv::Mat img, int height, int width) {
  const int offsetW = (img.cols - width) / 2;
  const int offsetH = (img.rows - height) / 2;
  const cv::Rect roi(offsetW, offsetH, width, height);
  img = img(roi);
  return img;
}

}  // namespace amdinfer::pre_post

#endif  // GUARD_AMDINFER_PRE_POST_CENTER_CROP
