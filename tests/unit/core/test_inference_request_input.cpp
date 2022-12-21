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

TEST(UnitInferenceRequestInput, SerDes) {
  const auto data_size = 10;
  std::vector<char> data;
  data.reserve(data_size);
  for (auto i = 0; i < data_size; i++) {
    data.push_back(static_cast<char>('a' + i));
  }

  InferenceRequestInput req(data.data(), {data_size}, DataType::Uint8, "test");

  std::vector<std::byte> serial_data;
  serial_data.reserve(req.serializeSize());
  req.serialize(serial_data.data());

  InferenceRequestInput new_req;
  new_req.deserialize(serial_data.data());

  EXPECT_EQ(req.getData(), new_req.getData());
  bool type_match = req.getDatatype() == new_req.getDatatype();
  EXPECT_TRUE(type_match);
  EXPECT_EQ(req.getName(), new_req.getName());
  auto old_shape = req.getShape();
  auto new_shape = new_req.getShape();
  const auto old_shape_size = old_shape.size();
  EXPECT_EQ(old_shape_size, new_shape.size());
  for (auto i = 0U; i < old_shape_size; ++i) {
    EXPECT_EQ(old_shape[i], new_shape[i]);
  }
  EXPECT_EQ(req.getSize(), new_req.getSize());
}

TEST(UnitInferenceRequestInput, SerDes2) {
  const auto data_size = 10;
  std::vector<std::byte> data;
  data.reserve(data_size);
  for (auto i = 0; i < data_size; ++i) {
    auto c = static_cast<std::byte>(static_cast<unsigned char>('a' + i));
    // std::cout << c << " ";
    data.push_back(c);
  }

  InferenceRequestInput req(nullptr, {data_size}, DataType::Uint8, "test");
  req.setData(std::move(data));

  std::vector<std::byte> serial_data;
  serial_data.reserve(req.serializeSize());
  req.serialize(serial_data.data());

  InferenceRequestInput new_req;
  new_req.deserialize(serial_data.data());

  EXPECT_NE(req.getData(), new_req.getData());
  const auto* old_data = static_cast<char*>(req.getData());
  const auto* new_data = static_cast<char*>(new_req.getData());
  for (int i = 0; i < data_size; i++) {
    EXPECT_EQ(old_data[i], new_data[i]);
  }
  bool type_match = req.getDatatype() == new_req.getDatatype();
  EXPECT_TRUE(type_match);
  EXPECT_EQ(req.getName(), new_req.getName());
  auto old_shape = req.getShape();
  auto new_shape = new_req.getShape();
  const auto old_shape_size = old_shape.size();
  EXPECT_EQ(old_shape_size, new_shape.size());
  for (auto i = 0U; i < old_shape_size; ++i) {
    EXPECT_EQ(old_shape[i], new_shape[i]);
  }
  EXPECT_EQ(req.getSize(), new_req.getSize());
}

}  // namespace amdinfer
