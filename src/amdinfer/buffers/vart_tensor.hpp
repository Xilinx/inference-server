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
 * @brief Defines the VartTensorBuffer class
 */

#ifndef GUARD_AMDINFER_BUFFERS_VART_TENSOR
#define GUARD_AMDINFER_BUFFERS_VART_TENSOR

#include <cstddef>  // for size_t

#include "amdinfer/buffers/buffer.hpp"

namespace vart {
class TensorBuffer;
}  // namespace vart

namespace amdinfer {

class VartTensorBuffer : public Buffer {
 public:
  /**
   * @brief Construct a new Vector Buffer object
   *
   * @param elements number of elements to store
   * @param data_type type of element to store
   */
  VartTensorBuffer(void* data, MemoryAllocators allocator);

  /**
   * @brief Returns a pointer to the underlying data
   *
   * @return void*
   */
  void* data(size_t offset) override;

 private:
  vart::TensorBuffer* data_;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_BUFFERS_VART_TENSOR
