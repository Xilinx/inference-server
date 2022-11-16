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
 * @brief Implements the VartTensorBuffer class
 */

#include "amdinfer/buffers/vart_tensor_buffer.hpp"

#include <utility>  // for pair

namespace amdinfer {

VartTensorBuffer::VartTensorBuffer(const std::string& name,
                                   std::vector<int32_t>& shape,
                                   xir::DataType data_type)
  : tensor_(xir::Tensor::create(name, shape, data_type)),
    data_(tensor_.get()) {}

VartTensorBuffer::VartTensorBuffer(const char* name,
                                   std::vector<int32_t>& shape,
                                   xir::DataType data_type)
  : tensor_(xir::Tensor::create(name, shape, data_type)),
    data_(tensor_.get()) {}

void* VartTensorBuffer::data(size_t offset) {
  // Some DPUs need a shape argument to data() to get the data properly.
  // This argument should be the same size as the tensor (by default,
  // [batch, h, w, c]). This first argument is the batch index. The other
  // indices should be zero to get the start of the batch
  std::vector<int> shape(this->tensor_->get_shape().size(), 0);

  auto dims = this->tensor_->get_shape();
  auto size = this->tensor_->get_shape().size();

  // convert the offset to a shape based on the tensor shape
  for (auto k = 0U; k < size; k++) {
    auto stride = 1;
    for (auto m = k + 1; m < size; m++) {
      stride *= dims[m];
    }
    shape[k] = offset / stride;
    offset -= shape[k] * stride;
  }

  return reinterpret_cast<void*>(this->data_.data(shape).first);
}

void VartTensorBuffer::reset() {}

vart::TensorBuffer* VartTensorBuffer::getTensorBuffer() { return &this->data_; }

}  // namespace amdinfer
