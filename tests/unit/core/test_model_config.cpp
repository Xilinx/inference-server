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

#include <toml++/toml.h>

#include <iostream>

#include "amdinfer/core/model_config.hpp"  // for ModelConfig
#include "gtest/gtest.h"                   // for Message, TestPartResult, Test

using namespace std::string_view_literals;

namespace amdinfer {

static constexpr std::string_view toml_str = R"(
  name = "mnist"
  platform = "tensorflow_graphdef"

  [[inputs]]
  name = "images_in"
  datatype = "FP32"
  shape = [28, 28, 1]

  [[outputs]]
  name = "flatten/Reshape"
  datatype = "FP32"
  shape = [10]
)"sv;

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitModelConfig, Construction) {
  const auto toml = toml::parse(toml_str);
  ModelConfig config{toml};

  EXPECT_EQ(config.name(), "mnist");
  EXPECT_EQ(config.platform(), "tensorflow_graphdef");

  const auto& inputs = config.inputs();
  EXPECT_EQ(inputs.size(), 1);
  const auto& input = inputs.at(0);
  EXPECT_EQ(input.getName(), "images_in");
  EXPECT_EQ(input.getDatatype(), DataType::Fp32);
  const auto& input_shape = input.getShape();
  EXPECT_EQ(input_shape.size(), 3);
  const std::vector<uint64_t> golden_input_shape{28, 28, 1};
  EXPECT_EQ(input_shape, golden_input_shape);

  const auto& outputs = config.outputs();
  EXPECT_EQ(outputs.size(), 1);
  const auto& output = outputs.at(0);
  EXPECT_EQ(output.getName(), "flatten/Reshape");
  EXPECT_EQ(output.getDatatype(), DataType::Fp32);
  const auto& output_shape = output.getShape();
  EXPECT_EQ(output_shape.size(), 1);
  const std::vector<uint64_t> golden_output_shape{10};
  EXPECT_EQ(output_shape, golden_output_shape);
}

}  // namespace amdinfer
