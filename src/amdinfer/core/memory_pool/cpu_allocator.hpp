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

#ifndef GUARD_AMDINFER_CORE_MEMORY_POOL_CPU_ALLOCATOR
#define GUARD_AMDINFER_CORE_MEMORY_POOL_CPU_ALLOCATOR

#include <cstddef>
#include <list>
#include <vector>

#include "amdinfer/core/memory_pool/memory_allocator.hpp"

namespace amdinfer {

struct MemoryHeader;

class CpuAllocator : public MemoryAllocator {
 public:
  explicit CpuAllocator(size_t block_size, size_t max_allocated = -1);

  [[nodiscard]] void* get(size_t size) override;
  void put(const void* address) override;

  // void free(const void* address);

 private:
  size_t allocated_ = 0;
  size_t max_allocate_;
  size_t block_size_;
  size_t block_id_ = 0;
  std::list<MemoryHeader> headers_;
  std::vector<std::vector<std::byte>> data_;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_MEMORY_POOL_CPU_ALLOCATOR
