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

void validateMetadata(const ModelConfig& config, const std::string& golden_name,
                      const std::string& golden_platform,
                      const std::string& golden_id) {
  EXPECT_EQ(config.name(), golden_name);
  EXPECT_EQ(config.platform(), golden_platform);
  EXPECT_EQ(config.id(), golden_id);
}

void validateTensor(const ModelConfigTensor& tensor,
                    const Tensor& golden_tensor, const std::string& id) {
  EXPECT_EQ(tensor.getName(), golden_tensor.getName());
  EXPECT_EQ(tensor.getDatatype(), golden_tensor.getDatatype());
  EXPECT_EQ(tensor.getShape(), golden_tensor.getShape());

  EXPECT_EQ(tensor.id(), id);
}

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitModelConfig, Basic) {
  constexpr std::string_view toml_str = R"(
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

  const auto toml = toml::parse(toml_str);
  ModelConfig config{toml};

  ASSERT_NO_FATAL_FAILURE(
    validateMetadata(config, "mnist", "tensorflow_graphdef", ""));

  const auto& inputs = config.inputs();
  EXPECT_EQ(inputs.size(), 1);
  const auto& input = inputs.at(0);

  Tensor golden_input{"images_in", {28, 28, 1}, DataType::Fp32};
  ASSERT_NO_FATAL_FAILURE(validateTensor(input, golden_input, ""));

  const auto& outputs = config.outputs();
  EXPECT_EQ(outputs.size(), 1);
  const auto& output = outputs.at(0);

  Tensor golden_output{"flatten/Reshape", {10}, DataType::Fp32};
  ASSERT_NO_FATAL_FAILURE(validateTensor(output, golden_output, ""));
}

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitModelConfig, SingleEnsemble) {
  constexpr std::string_view toml_str = R"(
    [[models]]
    name = "mnist"
    platform = "tensorflow_graphdef"
    id = "bar.pb"

    [[models.inputs]]
    name = "images_in"
    datatype = "FP32"
    shape = [28, 28, 1]
    id = "image"

    [[models.outputs]]
    name = "flatten/Reshape"
    datatype = "FP32"
    shape = [10]
    id = "classes"
  )"sv;

  const auto toml = toml::parse(toml_str);
  ModelConfig config{toml};

  ASSERT_NO_FATAL_FAILURE(
    validateMetadata(config, "mnist", "tensorflow_graphdef", "bar.pb"));

  const auto& inputs = config.inputs();
  EXPECT_EQ(inputs.size(), 1);
  const auto& input = inputs.at(0);

  Tensor golden_input{"images_in", {28, 28, 1}, DataType::Fp32};
  ASSERT_NO_FATAL_FAILURE(validateTensor(input, golden_input, "image"));

  const auto& outputs = config.outputs();
  EXPECT_EQ(outputs.size(), 1);
  const auto& output = outputs.at(0);

  Tensor golden_output{"flatten/Reshape", {10}, DataType::Fp32};
  ASSERT_NO_FATAL_FAILURE(validateTensor(output, golden_output, "classes"));
}

}  // namespace amdinfer
