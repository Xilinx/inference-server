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

/**
 * @file
 * @brief Defines the hard batcher implementation
 */

#ifndef GUARD_AMDINFER_BATCHING_HARD
#define GUARD_AMDINFER_BATCHING_HARD

#include "amdinfer/batching/batcher.hpp"  // IWYU pragma: export

namespace amdinfer {
class WorkerInfo;
}  // namespace amdinfer

namespace amdinfer {

/**
 * @brief The HardBatcher batches to a multiple of the requested batch size and
 * blocks until a valid batch forms. Note: this batcher is only meant for
 * testing purposes or if the batch size is fixed to be one since it can block
 * indefinitely.
 *
 */
class HardBatcher : public Batcher {
 public:
  using Batcher::Batcher;

 private:
  void doRun(WorkerInfo* worker) override;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_BATCHING_HARD
