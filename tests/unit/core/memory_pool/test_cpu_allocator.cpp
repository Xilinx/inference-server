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

#include "amdinfer/core/exceptions.hpp"
#include "amdinfer/core/memory_pool/cpu_allocator.hpp"  // for CpuSimpl...
#include "amdinfer/testing/gtest.hpp"  // for AssertionResult,...

namespace amdinfer {

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitCpuAllocator, Basic) {
  CpuAllocator allocator{sizeof(int)};

  const auto* foo = static_cast<int*>(allocator.get(sizeof(int)));

  const auto* bar = static_cast<int*>(allocator.get(sizeof(int)));
  ASSERT_NE(foo, bar);

  allocator.put(foo);
  allocator.put(bar);
}

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitCpuAllocator, Correctness) {
  CpuAllocator allocator{sizeof(int) * 4, sizeof(int) * 4};

  const auto* foo = static_cast<int*>(allocator.get(sizeof(int)));
  const auto* bar = static_cast<int*>(allocator.get(sizeof(int)));

  ASSERT_EQ(foo + 1, bar);

  allocator.put(foo);
  const auto* baz = static_cast<int*>(allocator.get(sizeof(int)));

  ASSERT_EQ(foo, baz);

  allocator.put(baz);
  allocator.put(bar);

  const auto* bard = static_cast<int*>(allocator.get(sizeof(int) * 4));
  ASSERT_EQ(foo, bard);
}

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitCpuAllocator, ExceedingMax) {
  CpuAllocator allocator{sizeof(int), sizeof(int)};

  std::ignore = static_cast<int*>(allocator.get(sizeof(int)));
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto, hicpp-avoid-goto)
  EXPECT_THROW_CHECK(
    std::ignore = static_cast<int*>(allocator.get(sizeof(int)));
    , EXPECT_STREQ(e.what(), "Too much requested");, runtime_error);
}

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitCpuAllocator, BadFree) {
  CpuAllocator allocator{sizeof(int)};

  const auto* foo = static_cast<int*>(allocator.get(sizeof(int)));
  const auto* bad_address = foo + 1;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto, hicpp-avoid-goto)
  EXPECT_THROW_CHECK(allocator.put(bad_address);
                     , EXPECT_STREQ(e.what(), "Address not found");
                     , runtime_error);
}

}  // namespace amdinfer
