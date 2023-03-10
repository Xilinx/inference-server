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

#include "amdinfer/core/memory_pool/pool.hpp"

#include "amdinfer/buffers/cpu.hpp"
#include "amdinfer/core/exceptions.hpp"
#include "amdinfer/core/memory_pool/cpu_allocator.hpp"
#include "amdinfer/core/memory_pool/vart_tensor_allocator.hpp"

namespace amdinfer {

const size_t kDefaultCpuBlockSize = 1'048'576;  // arbitrarily 1MiB

MemoryPool::MemoryPool() {
  allocators_.try_emplace(MemoryAllocators::Cpu,
                          std::make_unique<CpuAllocator>(kDefaultCpuBlockSize));
#ifdef AMDINFER_ENABLE_VITIS
  allocators_.try_emplace(MemoryAllocators::VartTensor,
                          std::make_unique<VartTensorAllocator>());
#endif
}

std::unique_ptr<Buffer> MemoryPool::get(
  const std::vector<MemoryAllocators>& allocators,
  const InferenceRequestInput& tensor, size_t batch_size) const {
  for (const auto& allocator : allocators) {
    try {
      return allocators_.at(allocator)->get(tensor, batch_size);
    } catch (const runtime_error&) {
      continue;
    }
  }
  throw runtime_error("Memory could not be allocated");
}

void MemoryPool::put(std::unique_ptr<Buffer> memory) const {
  const auto allocator = memory->getAllocator();
  const auto* address = memory->data(0);
  allocators_.at(allocator)->put(address);
}

}  // namespace amdinfer
