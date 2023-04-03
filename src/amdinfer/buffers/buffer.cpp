// Copyright 2021 Xilinx, Inc.
// Copyright 2022 Advanced Micro Devices, Inc.
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
 * @brief Implements the Buffer class
 */

#include "amdinfer/buffers/buffer.hpp"

#include <cstring>  // for memcpy, size_t

#include "amdinfer/core/memory_pool/pool.hpp"

namespace amdinfer {

Buffer::Buffer(MemoryAllocators allocator) : allocator_(allocator) {}

size_t Buffer::write(void* data, size_t offset, size_t size) {
  std::memcpy(this->data(offset), data, size);
  return offset + size;
}

MemoryAllocators Buffer::getAllocator() const { return allocator_; }

void Buffer::setPool(const MemoryPool* pool) { pool_ = pool; }

const MemoryPool* Buffer::getPool() const { return pool_; }

}  // namespace amdinfer
