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
 * @brief Implements the CpuBuffer class
 */

#include "amdinfer/buffers/cpu.hpp"

#include <algorithm>  // for max
#include <memory>     // for make_unique
#include <utility>    // for move

namespace amdinfer {

CpuBuffer::CpuBuffer(void* data, MemoryAllocators allocator)
  : allocator_(allocator), data_(static_cast<std::byte*>(data)) {}

void* CpuBuffer::data(size_t offset) { return data_ + offset; }
void CpuBuffer::reset() {}

}  // namespace amdinfer
