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

#ifndef GUARD_AMDINFER_PRE_POST_RESNET50_POSTPROCESS
#define GUARD_AMDINFER_PRE_POST_RESNET50_POSTPROCESS

#include <vector>

#include "amdinfer/pre_post/get_top_k.hpp"
#include "amdinfer/pre_post/softmax.hpp"

namespace amdinfer::pre_post {

/**
 * @brief Perform postprocessing of the data
 *
 * @tparam T the expected type of the data
 * @param output output from the server
 * @param k number of top elements to return
 * @return std::vector<int>
 */
template <typename T>
std::vector<int> resnet50Postprocess(const T* data, size_t size, int k) {
  std::vector<double> softmax;
  softmax.resize(size, 0);

  calc_softmax(data, size, softmax.data());
  return get_top_k(softmax.data(), size, k);
}

}  // namespace amdinfer::pre_post

#endif  // GUARD_AMDINFER_PRE_POST_RESNET50_POSTPROCESS
