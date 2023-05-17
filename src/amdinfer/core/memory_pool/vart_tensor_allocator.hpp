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

#ifndef GUARD_AMDINFER_CORE_MEMORY_POOL_VART_TENSOR_ALLOCATOR
#define GUARD_AMDINFER_CORE_MEMORY_POOL_VART_TENSOR_ALLOCATOR

#include "amdinfer/build_options.hpp"

#ifdef AMDINFER_ENABLE_VITIS

#include <cstddef>
#include <list>
#include <mutex>
#include <vart/experimental/runner_helper.hpp>  // for CpuFlatTensorBufferOwned
#include <vector>
#include <xir/tensor/tensor.hpp>  // for Tensor

#include "amdinfer/core/data_types.hpp"
#include "amdinfer/core/memory_pool/memory_allocator.hpp"

namespace amdinfer {

struct VartTensorHeader {
  std::byte* address;
  bool free;

  std::string name;
  std::vector<int64_t> shape;
  DataType datatype;
  size_t batch_size;

  VartTensorHeader(std::byte* address, bool free, const std::string& name,
                   const std::vector<int64_t>& shape, DataType datatype,
                   size_t batch_size)
    : address(address),
      free(free),
      name(name),
      shape(shape),
      datatype(datatype),
      batch_size(batch_size) {}
};

class VartTensorAllocator : public MemoryAllocator {
 public:
  explicit VartTensorAllocator(size_t max_allocated = -1);

  [[nodiscard]] BufferPtr get(const Tensor& tensor, size_t batch_size) override;
  void put(const void* address) override;

  // void free(const void* address);

 private:
  size_t allocated_ = 0;
  size_t max_allocate_;
  std::mutex mutex_;

  std::list<VartTensorHeader> headers_;
  std::list<std::unique_ptr<xir::Tensor>> tensors_;
  std::list<vart::CpuFlatTensorBufferOwned> buffers_;
};

}  // namespace amdinfer

#endif

#endif  // GUARD_AMDINFER_CORE_MEMORY_POOL_VART_TENSOR_ALLOCATOR
