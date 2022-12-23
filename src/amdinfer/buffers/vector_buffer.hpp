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
 * @brief Defines the VectorBuffer class
 */

#ifndef GUARD_AMDINFER_BUFFERS_VECTOR_BUFFER
#define GUARD_AMDINFER_BUFFERS_VECTOR_BUFFER

#include <cstddef>  // for size_t, byte
#include <vector>   // for vector

#include "amdinfer/buffers/buffer.hpp"   // IWYU pragma: export
#include "amdinfer/core/data_types.hpp"  // for DataType
#include "amdinfer/util/queue.hpp"       // for BufferPtrsQueue

namespace amdinfer {

/**
 * @brief VectorBuffer uses vectors for storing data
 *
 */
class VectorBuffer : public Buffer {
 public:
  /**
   * @brief Construct a new Vector Buffer object
   *
   * @param elements number of elements to store
   * @param data_type type of element to store
   */
  VectorBuffer(int elements, DataType data_type);

  /**
   * @brief Returns a pointer to the underlying data. Vectors guarantee that it
   * is contiguous.
   *
   * @return void*
   */
  void* data(size_t offset) override;

  /**
   * @brief Reset the internal write_counter_ prior to returning the buffer to
   * a pool
   */
  void reset() override;

  /**
   * @brief Allocate VectorBuffers in the given BufferPtrsQueue
   *
   * @param my_buffer a queue of Buffer pointers
   * @param num number of VectorBuffers to allocate
   * @param elements number of elements per VectorBuffer
   * @param data_type data type of the VectorBuffer (used for sizing)
   */
  static void allocate(BufferPtrsQueue* my_buffer, size_t num, size_t elements,
                       DataType data_type);

 private:
  /// The type of the data is used to size the vector appropriately
  DataType type_;
  /// Our actual data is allocated as bytes to support different types
  std::vector<std::byte> data_;
  /// Counter to track the next index to write at
  // size_t write_counter_ = 0;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_BUFFERS_VECTOR_BUFFER
