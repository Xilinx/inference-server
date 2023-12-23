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

#ifndef GUARD_AMDINFER_PRE_POST_MNIST_POSTPROCESS
#define GUARD_AMDINFER_PRE_POST_MNIST_POSTPROCESS

#include <vector>

#include "amdinfer/pre_post/get_top_k.hpp"
#include "amdinfer/pre_post/softmax.hpp"

namespace amdinfer::pre_post {

/**
 * @brief Perform postprocessing of the output of mnist classifer
 *
 * @param data output from the server
 * @param size number of elements in the output data
 * @return std::vector<int>
 */

std::vector<int> mnistPostprocess(const float* data, size_t size) {
  double dataVec[10] = {0.0};
  for (size_t i = 0; i < size; i++) {
    dataVec[i] = static_cast<double>(data[i]);
  }
  return getTopK(dataVec, size, 1);
}

}  // namespace amdinfer::pre_post

#endif  // GUARD_AMDINFER_PRE_POST_RESNET50_POSTPROCESS
