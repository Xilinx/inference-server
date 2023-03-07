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

#ifndef GUARD_AMDINFER_CORE_MEMORY_POOL_POOL
#define GUARD_AMDINFER_CORE_MEMORY_POOL_POOL

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

#include "amdinfer/buffers/buffer.hpp"
#include "amdinfer/build_options.hpp"
#include "amdinfer/core/memory_pool/memory_allocator.hpp"

namespace amdinfer {

enum class MemoryAllocators { Cpu };

using Memory = std::pair<MemoryAllocators, void*>;

class MemoryPool {
 public:
  MemoryPool();

  std::unique_ptr<Buffer> get(const std::vector<MemoryAllocators>& allocators,
                              size_t size) const;
  void put(std::unique_ptr<Buffer> memory) const;

 private:
  std::unordered_map<MemoryAllocators, std::unique_ptr<MemoryAllocator>>
    allocators_;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_MEMORY_POOL_POOL
