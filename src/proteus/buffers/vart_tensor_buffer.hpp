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

#ifndef GUARD_PROTEUS_BUFFERS_VART_TENSOR_BUFFER
#define GUARD_PROTEUS_BUFFERS_VART_TENSOR_BUFFER

#include <cstdint>
#include <string>
#include <vart/experimental/runner_helper.hpp>
#include <vector>

#include "proteus/buffers/buffer.hpp"
#include "proteus/core/data_types.hpp"

namespace proteus {

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

  vart::TensorBuffer* getTensorBuffer();

 private:
  std::unique_ptr<xir::Tensor> tensor_;
  vart::CpuFlatTensorBufferOwned data_;
};

}  // namespace proteus

#endif  // GUARD_PROTEUS_BUFFERS_VART_TENSOR_BUFFER
