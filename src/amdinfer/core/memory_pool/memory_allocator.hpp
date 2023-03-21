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
 * @brief
 */

#ifndef GUARD_AMDINFER_CORE_MEMORY_POOL_MEMORY_ALLOCATOR
#define GUARD_AMDINFER_CORE_MEMORY_POOL_MEMORY_ALLOCATOR

#include <cstddef>  // for size_t

#include "amdinfer/core/tensor.hpp"
#include "amdinfer/declarations.hpp"

namespace amdinfer {

enum class MemoryAllocators { Cpu, VartTensor };

struct MemoryHeader {
  std::byte* address;
  bool free;
  size_t size;
  size_t block_id;

  MemoryHeader(std::byte* address, size_t size, bool free, size_t block_id)
    : address(address), free(free), size(size), block_id(block_id) {}
};

class MemoryAllocator {
 public:
  virtual ~MemoryAllocator() = default;

  // these methods are thread-safe
  [[nodiscard]] virtual BufferPtr get(const Tensor& tensor,
                                      size_t batch_size) = 0;
  virtual void put(const void* address) = 0;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_MEMORY_POOL_MEMORY_ALLOCATOR
