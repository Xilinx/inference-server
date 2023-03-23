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

#ifndef GUARD_AMDINFER_BUFFERS_VECTOR
#define GUARD_AMDINFER_BUFFERS_VECTOR

#include <cstddef>  // for size_t, byte
#include <vector>   // for vector

#include "amdinfer/buffers/buffer.hpp"   // IWYU pragma: export
#include "amdinfer/core/data_types.hpp"  // for DataType
#include "amdinfer/util/queue.hpp"       // for BufferPtrsQueue

namespace amdinfer {

enum class MemoryAllocators;

/**
 * @brief CpuBuffer uses vectors for storing data
 *
 */
class VectorBuffer : public Buffer {
 public:
  /**
   * @brief Construct a new Vector Buffer object
   *
   * @param size size of the buffer in bytes
   */
  explicit VectorBuffer(size_t size);

  /**
   * @brief Returns a pointer to the underlying data
   *
   * @return void*
   */
  void* data(size_t offset) override;

 private:
  std::vector<std::byte> data_;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_BUFFERS_VECTOR
