// Copyright 2022 Xilinx Inc.
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

#ifndef GUARD_PROTEUS_UTIL_PRE_POST_GET_TOP_K
#define GUARD_PROTEUS_UTIL_PRE_POST_GET_TOP_K

#include <queue>
#include <vector>

namespace proteus::util {

/**
 * @brief After running softmax, get the labels associated with the top k values
 *
 * @param d pointer to the data
 * @param size number of elements in the data
 * @param k number of top elements to return
 * @return std::vector<int>
 */
std::vector<int> get_top_k(const double* d, size_t size, int k) {
  std::priority_queue<std::pair<float, int>> q;

  for (auto i = 0U; i < size; ++i) {
    q.push(std::pair<float, int>(d[i], i));
  }

  std::vector<int> topKIndex;
  topKIndex.reserve(k);
  for (auto i = 0; i < k; ++i) {
    const auto& [prob, index] = q.top();
    topKIndex.push_back(index);
    q.pop();
  }
  return topKIndex;
}

}  // namespace proteus::util

#endif  // GUARD_PROTEUS_UTIL_PRE_POST_GET_TOP_K
