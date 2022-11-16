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

#ifndef GUARD_PROTEUS_UTIL_QUEUE
#define GUARD_PROTEUS_UTIL_QUEUE

#include <concurrentqueue/blockingconcurrentqueue.h>  // IWYU pragma: export

#include <memory>
#include <vector>

#include "amdinfer/declarations.hpp"

namespace amdinfer {

template <class T>
using BlockingQueue = moodycamel::BlockingConcurrentQueue<T>;

using BufferPtrsQueue = BlockingQueue<BufferPtrs>;
using BufferPtrsQueuePtr = std::unique_ptr<BufferPtrsQueue>;

using InferenceRequestPtrQueue = BlockingQueue<InferenceRequestPtr>;

}  // namespace amdinfer

#endif  // GUARD_PROTEUS_UTIL_QUEUE
