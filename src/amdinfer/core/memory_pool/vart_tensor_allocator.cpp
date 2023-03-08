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

#include "amdinfer/core/memory_pool/vart_tensor_allocator.hpp"

#include <cassert>
#include <iostream>
#include <vector>
#include <xir/util/data_type.hpp>  // for DataType

#include "amdinfer/buffers/vart_tensor.hpp"
#include "amdinfer/core/exceptions.hpp"
#include "amdinfer/core/predict_api.hpp"

namespace amdinfer {

xir::DataType mapTypeToXir(DataType type) {
  xir::DataType retval;
  auto bit_width = static_cast<int32_t>(type.size()) * 8;
  switch (type) {
    case DataType::Bool:
    case DataType::Uint8:
    case DataType::Uint16:
    case DataType::Uint32:
    case DataType::Uint64:
      retval.type = xir::DataType::UINT;
      break;
    case DataType::Int8:
    case DataType::Int16:
    case DataType::Int32:
    case DataType::Int64:
      retval.type = xir::DataType::INT;
      break;
    // case DataType::FP16 fall through to default handler
    case DataType::Fp32:
    case DataType::Fp64:
      retval.type = xir::DataType::FLOAT;
      break;
    default:
      throw invalid_argument("Unsupported type conversion to XIR");
  }
  retval.bit_width = bit_width;
  return retval;
}

VartTensorAllocator::VartTensorAllocator(size_t max_allocate)
  : max_allocate_(max_allocate) {}

BufferPtr VartTensorAllocator::get(const InferenceRequestInput& tensor,
                                   size_t batch_size) {
  const auto& name = tensor.getName();
  const auto& shape = tensor.getShape();
  const auto datatype = tensor.getDatatype();

  const std::lock_guard lock{mutex_};
  auto found = headers_.end();
  const auto end = headers_.end();
  for (auto it = headers_.begin(); it != end; it++) {
    if (it->free && it->name == name && it->shape == shape &&
        it->datatype == datatype && it->batch_size == batch_size) {
      found = it;
      break;
    }
  }

  if (found != end) {
    found->free = false;
    return std::make_unique<VartTensorBuffer>(found->address,
                                              MemoryAllocators::VartTensor);
  }

  auto size_to_allocate = tensor.getSize() * datatype.size() * batch_size;
  if (allocated_ + size_to_allocate > max_allocate_) {
    throw runtime_error("Too much requested");
  }

  auto xir_type = mapTypeToXir(datatype);
  std::vector<int> xir_shape{shape.begin(), shape.end()};
  tensors_.emplace_back(xir::Tensor::create(name, xir_shape, xir_type));
  buffers_.emplace_back(tensors_.back().get());

  allocated_ += size_to_allocate;

  auto* retval = reinterpret_cast<std::byte*>(&(buffers_.back()));

  headers_.emplace_back(retval, false, name, shape, datatype, batch_size);

  return std::make_unique<VartTensorBuffer>(retval,
                                            MemoryAllocators::VartTensor);
}

void VartTensorAllocator::put(const void* address) {
  const std::lock_guard lock{mutex_};
  const auto end = headers_.end();
  auto found = headers_.end();
  for (auto it = headers_.begin(); it != end; it++) {
    if (it->address == address) {
      found->free = true;
      // std::cout << "Freed memory\n";
      return;
    }
  }

  if (found == end) {
    throw runtime_error("Address not found");
  }
}

}  // namespace amdinfer
