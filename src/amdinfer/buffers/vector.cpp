// Copyright 2023 Advanced Micro Devices, Inc.
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
 * @brief Implements the VectorBuffer class
 */

#include "amdinfer/buffers/vector.hpp"

#include <algorithm>  // for max
#include <memory>     // for make_unique
#include <utility>    // for move

namespace amdinfer {

VectorBuffer::VectorBuffer(size_t size) : Buffer(MemoryAllocators::Cpu) {
  data_.resize(size);
}

void* VectorBuffer::data(size_t offset) { return data_.data() + offset; }

}  // namespace amdinfer