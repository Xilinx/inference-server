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

#include "amdinfer/core/memory_pool/cpu_allocator.hpp"

#include <cassert>
#include <iostream>
#include <vector>

#include "amdinfer/buffers/cpu.hpp"
#include "amdinfer/core/exceptions.hpp"
#include "amdinfer/core/inference_request.hpp"

namespace amdinfer {

CpuAllocator::CpuAllocator(size_t block_size, size_t max_allocate)
  : max_allocate_(max_allocate), block_size_(block_size) {}

BufferPtr CpuAllocator::get(const InferenceRequestInput& tensor,
                            size_t batch_size) {
  auto size = tensor.getSize() * tensor.getDatatype().size() * batch_size;

  const std::lock_guard lock{mutex_};
  auto best = headers_.end();
  const auto end = headers_.end();
  for (auto it = headers_.begin(); it != end; it++) {
    // if this block can hold this data and it's smaller than our current best
    if (it->free && it->size >= size &&
        (best == end || it->size < best->size)) {
      best = it;
    }
  }

  if (best != end) {
    if (best->size == size) {
      best->free = false;
      // std::cout << "Matched " << size << " bytes\n";
      return std::make_unique<CpuBuffer>(best->address, MemoryAllocators::Cpu);
    }
    const auto& new_block =
      headers_.emplace(best, best->address, size, false, best->block_id);
    assert(end == headers_.end());
    best->size -= size;
    best->address += size;
    // std::cout << "Partitioned " << size << " bytes\n";
    return std::make_unique<CpuBuffer>(new_block->address,
                                       MemoryAllocators::Cpu);
  }

  auto size_to_allocate = std::max(size, block_size_);
  if (allocated_ + size_to_allocate > max_allocate_) {
    throw runtime_error("Too much requested");
  }
  auto& new_block = data_.emplace_back();
  new_block.resize(size_to_allocate);
  allocated_ += size_to_allocate;

  auto* retval = new_block.data();
  block_id_++;

  headers_.emplace_back(retval, size, false, block_id_);
  assert(end == headers_.end());
  if (size < block_size_) {
    headers_.emplace_back(retval + size, block_size_ - size, true, block_id_);
    assert(end == headers_.end());
    // foo.next = std::prev(end);
  }

  // std::cout << "Allocated " << size << " bytes\n";
  return std::make_unique<CpuBuffer>(retval, MemoryAllocators::Cpu);
}

void CpuAllocator::put(const void* address) {
  const std::lock_guard lock{mutex_};
  const auto end = headers_.end();
  auto found = headers_.end();
  for (auto it = headers_.begin(); it != end; it++) {
    if (it->address == address) {
      found = it;
      break;
    }
  }

  if (found == end) {
    throw runtime_error("Address not found");
  }

  // if the previous is free and from the same block, merge the two
  if (found != std::begin(headers_)) {
    auto prev = std::prev(found);
    if (prev->block_id == found->block_id && prev->free) {
      prev->size += found->size;
      headers_.erase(found);
      found = prev;
    }
  }

  // if the next is free and from the same block, merge the two
  if (found != std::prev(std::end(headers_))) {
    auto next = std::next(found);
    if (next->block_id == found->block_id && next->free) {
      next->size += found->size;
      next->address = found->address;
      headers_.erase(found);
    } else {
      found->free = true;
    }
  } else {
    found->free = true;
  }

  // std::cout << "Freed memory\n";
}

}  // namespace amdinfer
