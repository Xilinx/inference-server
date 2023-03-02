// Copyright 2021 Xilinx, Inc.
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
 * @brief Defines the soft batcher implementation
 */

#ifndef GUARD_AMDINFER_BATCHING_SOFT
#define GUARD_AMDINFER_BATCHING_SOFT

#include "amdinfer/batching/batcher.hpp"  // IWYU pragma: export

namespace amdinfer {
enum class MemoryAllocators;
class WorkerInfo;
}  // namespace amdinfer

namespace amdinfer {

/**
 * @brief The SoftBatcher attempts to batch requests to the requested batch size
 * but has a timeout that passes an incomplete batch onwards if the batch cannot
 * be completed.
 *
 */
class SoftBatcher : public Batcher {
 public:
  using Batcher::Batcher;

 private:
  void doRun(const std::vector<MemoryAllocators>& allocators) override;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_BATCHING_SOFT
