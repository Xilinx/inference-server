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
 * @brief Defines the fake batcher implementation
 */

#ifndef GUARD_PROTEUS_BATCHING_FAKE_BATCHER
#define GUARD_PROTEUS_BATCHING_FAKE_BATCHER

#include "proteus/batching/batcher.hpp"      // IWYU pragma: export
#include "proteus/helpers/declarations.hpp"  // for InferenceResponseFuture...

namespace proteus {
class InferenceRequestInput;
class WorkerInfo;
}  // namespace proteus

namespace proteus {

class FakeBatcher : public Batcher {
 public:
  using Batcher::Batcher;
  void run(WorkerInfo* worker) override;
  InferenceResponseFuture enqueue(InferenceRequestInput request);
};

}  // namespace proteus

#endif  // GUARD_PROTEUS_BATCHING_FAKE_BATCHER
