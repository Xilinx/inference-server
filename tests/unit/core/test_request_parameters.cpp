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

#include <cstddef>
#include <iostream>

#include "amdinfer/core/predict_api.hpp"
#include "gtest/gtest.h"  // for Test, SuiteApiResolver, TEST

namespace amdinfer {

TEST(UnitRequestParameters, SerDes) {
  std::array<std::string, 4> keys = {"string", "int", "bool", "double"};

  RequestParameters params;
  params.put(keys[0], "test");
  params.put(keys[1], 4);
  params.put(keys[2], false);
  params.put(keys[3], 0.4);

  auto size = params.serializeSize();
  // 1 for number of parameters, plus 3 for each parameter: type index, length
  // of the key, and length of the data
  size_t expected_size = (1 + 3 * params.size()) * sizeof(size_t);
  expected_size += keys[0].size() + (params.get<std::string>(keys[0]).size());
  expected_size += keys[1].size() + sizeof(int);
  expected_size += keys[2].size() + sizeof(bool);
  expected_size += keys[3].size() + sizeof(double);
  EXPECT_EQ(size, expected_size);

  std::vector<std::byte> data;
  data.reserve(size);
  params.serialize(data.data());

  RequestParameters new_params;
  ASSERT_TRUE(new_params.empty());
  new_params.deserialize(data.data());
  EXPECT_EQ(params.size(), new_params.size());

  for (const auto& key : keys) {
    EXPECT_TRUE(new_params.has(key));
  }
  EXPECT_EQ(params.get<std::string>(keys[0]),
            new_params.get<std::string>(keys[0]));
  EXPECT_EQ(params.get<int>(keys[1]), new_params.get<int>(keys[1]));
  EXPECT_EQ(params.get<bool>(keys[2]), new_params.get<bool>(keys[2]));
  EXPECT_EQ(params.get<double>(keys[3]), new_params.get<double>(keys[3]));
}

}  // namespace amdinfer
