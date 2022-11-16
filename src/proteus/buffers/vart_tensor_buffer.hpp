// Copyright 2021 Xilinx Inc.
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

#ifndef GUARD_AMDINFER_BUFFERS_VART_TENSOR_BUFFER
#define GUARD_AMDINFER_BUFFERS_VART_TENSOR_BUFFER

#include <stddef.h>  // for size_t

#include <cstdint>                              // for int32_t
#include <memory>                               // for unique_ptr
#include <string>                               // for string
#include <vart/experimental/runner_helper.hpp>  // for CpuFlatTensorBufferOwned
#include <vector>                               // for vector
#include <xir/tensor/tensor.hpp>                // for Tensor
#include <xir/util/data_type.hpp>               // for DataType

#include "amdinfer/buffers/buffer.hpp"

namespace vart {
class TensorBuffer;
}

namespace amdinfer {

/**
 * @brief VartTensorBuffer uses vart::CpuFlatTensorBufferOwned for storing data
 *
 */
class VartTensorBuffer : public Buffer {
 public:
  VartTensorBuffer(const std::string& name, std::vector<int32_t>& shape,
                   xir::DataType data_type);
  VartTensorBuffer(const char* name, std::vector<int32_t>& shape,
                   xir::DataType data_type);

  /**
   * @brief Returns a pointer to the underlying data.
   *
   * @param index Used to index to the correct tensor in the batch
   *
   * @return void*
   */
  void* data(size_t offset) override;

  /**
   * @brief Perform any cleanup before returning the buffer to the pool
   */
  void reset() override;

  /// Get a pointer to the underlying TensorBuffer
  vart::TensorBuffer* getTensorBuffer();

 private:
  std::unique_ptr<xir::Tensor> tensor_;
  vart::CpuFlatTensorBufferOwned data_;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_BUFFERS_VART_TENSOR_BUFFER
