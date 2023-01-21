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

#include <array>   // for array
#include <memory>  // for allocator

#include "amdinfer/util/compression.hpp"  // for zDecompress
#include "gtest/gtest.h"                  // for Test, SuiteApiResolver, EXP...

namespace amdinfer {

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitUtilCompression, Decompression) {
  // compressed "amdinfer" with zlib in Python
  const std::array<unsigned char, 16> compressed_data{
    0x78, 0x9c, 0x4b, 0xcc, 0x4d, 0xc9, 0xcc, 0x4b,
    0x4b, 0x2d, 0x2,  0x0,  0xe,  0x96, 0x3,  0x47};

  const auto* data = reinterpret_cast<const char*>(compressed_data.data());

  auto decompressed_str = util::zDecompress(data, compressed_data.size());

  EXPECT_EQ(decompressed_str, "amdinfer");
}

}  //  namespace amdinfer
