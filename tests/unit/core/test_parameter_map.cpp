// Copyright 2022 Xilinx, Inc.
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

#include <array>    // for array
#include <cstddef>  // for byte, size_t
#include <string>   // for string, basic_string, alloc...
#include <vector>   // for vector

#include "amdinfer/core/parameters.hpp"  // for ParameterMap
#include "gtest/gtest.h"                 // for AssertionResult, Message

namespace amdinfer {

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitParameterMap, SerDes) {
  std::array<std::string, 4> keys = {"string", "int", "bool", "double"};
  const auto index_string = 0;
  const auto index_int = 1;
  const auto index_bool = 2;
  const auto index_double = 3;

  // using arbitrary parameter values. Only the type matters
  const int param_int = 4;
  const double param_double = 0.4;

  ParameterMap params;
  params.put(keys[index_string], "test");
  params.put(keys[index_int], param_int);
  params.put(keys[index_bool], false);
  params.put(keys[index_double], param_double);

  auto size = params.serializeSize();
  // 1 for number of parameters, plus 3 for each parameter: type index, length
  // of the key, and length of the data
  size_t expected_size = (1 + 3 * params.size()) * sizeof(size_t);
  expected_size += keys[index_string].size() +
                   (params.get<std::string>(keys[index_string]).size());
  expected_size += keys[index_int].size() + sizeof(int);
  expected_size += keys[index_bool].size() + sizeof(bool);
  expected_size += keys[index_double].size() + sizeof(double);
  EXPECT_EQ(size, expected_size);

  std::vector<std::byte> data;
  data.reserve(size);
  params.serialize(data.data());

  ParameterMap new_params;
  ASSERT_TRUE(new_params.empty());
  new_params.deserialize(data.data());
  EXPECT_EQ(params.size(), new_params.size());

  for (const auto& key : keys) {
    EXPECT_TRUE(new_params.has(key));
  }
  EXPECT_EQ(params.get<std::string>(keys[index_string]),
            new_params.get<std::string>(keys[index_string]));
  EXPECT_EQ(params.get<int>(keys[index_int]),
            new_params.get<int>(keys[index_int]));
  EXPECT_EQ(params.get<bool>(keys[index_bool]),
            new_params.get<bool>(keys[index_bool]));
  EXPECT_EQ(params.get<double>(keys[index_double]),
            new_params.get<double>(keys[index_double]));
}

}  // namespace amdinfer
