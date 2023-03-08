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

// #include <array>    // for array
// #include <cstddef>  // for byte, size_t
// #include <string>   // for string, basic_string, alloc...
// #include <vector>   // for vector

#include "amdinfer/buffers/buffer.hpp"  // for BufferPtr
#include "amdinfer/core/exceptions.hpp"
#include "amdinfer/core/memory_pool/cpu_allocator.hpp"  // for CpuSimpl...
#include "amdinfer/core/predict_api.hpp"  // for InferenceRequestInput
#include "amdinfer/testing/gtest.hpp"     // for AssertionResult,...

namespace amdinfer {

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitCpuAllocator, Basic) {
  CpuAllocator allocator{sizeof(int)};

  InferenceRequestInput input{nullptr, {1}, DataType::Int32};

  const auto buffer_0 = allocator.get(input, 1);
  const auto buffer_1 = allocator.get(input, 1);

  const auto* address_0 = static_cast<int*>(buffer_0->data(0));
  const auto* address_1 = static_cast<int*>(buffer_1->data(0));
  ASSERT_NE(address_0, address_1);

  allocator.put(address_0);
  allocator.put(address_1);
}

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitCpuAllocator, Correctness) {
  CpuAllocator allocator{sizeof(int) * 4, sizeof(int) * 4};
  InferenceRequestInput input{nullptr, {1}, DataType::Int32};

  const auto buffer_0 = allocator.get(input, 1);
  const auto buffer_1 = allocator.get(input, 1);

  const auto* address_0 = static_cast<int*>(buffer_0->data(0));
  const auto* address_1 = static_cast<int*>(buffer_1->data(0));

  ASSERT_EQ(address_0 + 1, address_1);

  allocator.put(address_0);
  const auto buffer_2 = allocator.get(input, 1);
  const auto* address_2 = static_cast<int*>(buffer_2->data(0));

  ASSERT_EQ(address_0, address_2);

  allocator.put(address_2);
  allocator.put(address_1);

  const auto buffer_3 = allocator.get(input, 4);
  const auto* address_3 = static_cast<int*>(buffer_3->data(0));
  ASSERT_EQ(address_0, address_3);
}

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitCpuAllocator, ExceedingMax) {
  CpuAllocator allocator{sizeof(int), sizeof(int)};
  InferenceRequestInput input{nullptr, {1}, DataType::Int32};

  std::ignore = allocator.get(input, 1);
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto, hicpp-avoid-goto)
  EXPECT_THROW_CHECK(std::ignore = allocator.get(input, 1);
                     , EXPECT_STREQ(e.what(), "Too much requested");
                     , runtime_error);
}

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitCpuAllocator, BadFree) {
  CpuAllocator allocator{sizeof(int)};
  InferenceRequestInput input{nullptr, {1}, DataType::Int32};

  const auto buffer_0 = allocator.get(input, 1);
  const auto* address_0 = static_cast<int*>(buffer_0->data(0));
  const auto* bad_address = address_0 + 1;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto, hicpp-avoid-goto)
  EXPECT_THROW_CHECK(allocator.put(bad_address);
                     , EXPECT_STREQ(e.what(), "Address not found");
                     , runtime_error);
}

}  // namespace amdinfer
