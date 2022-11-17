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

#ifndef GUARD_AMDINFER_PRE_POST_SOFTMAX
#define GUARD_AMDINFER_PRE_POST_SOFTMAX

#include <cstddef>

namespace amdinfer::pre_post {

/**
 * @brief Calculate softmax of the data
 *
 * @tparam T the expected type of the data
 * @param data pointer to the raw data
 * @param size number of elements in the raw data
 * @param result pointer to store the computed results
 */
template <typename T>
void calc_softmax(const T* data, size_t size, double* result) {
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

}  // namespace amdinfer::pre_post

#endif  // GUARD_AMDINFER_PRE_POST_SOFTMAX
