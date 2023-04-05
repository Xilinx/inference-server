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

void validateMetadata(const ModelConfig& config, size_t index,
                      const std::string& golden_name,
                      const std::string& golden_platform,
                      const std::string& golden_id) {
  EXPECT_EQ(config.name(index), golden_name);
  EXPECT_EQ(config.platform(index), golden_platform);
  EXPECT_EQ(config.id(index), golden_id);
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
    validateMetadata(config, 0, "mnist", "tensorflow_graphdef", ""));

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
    validateMetadata(config, 0, "mnist", "tensorflow_graphdef", "bar.pb"));

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

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitModelConfig, Chain) {
  constexpr std::string_view toml_str = R"(
    [[models]]
    name = "preprocess"
    platform = "amdinfer_cpp"
    id = "base64_decode.so"

    [[models.inputs]]
    name = "image_in"
    datatype = "INT8"
    shape = [1080, 1920, 3]
    id = ""

    [[models.outputs]]
    name = "image_out"
    datatype = "INT8"
    shape = [1080, 1920, 3]
    id = "preprocessed_image"

    [[models]]
    name = "invert_image"
    platform = "amdinfer_cpp"
    id = "invert_image.so"

    [[models.inputs]]
    name = "image_in"
    datatype = "INT8"
    shape = [1080, 1920, 3]
    id = "preprocessed_image"

    [[models.outputs]]
    name = "image_out"
    datatype = "INT8"
    shape = [1080, 1920, 3]
    id = "inverted_image"

    [[models]]
    name = "postprocess"
    platform = "amdinfer_cpp"
    id = "base64_encode.so"

    [[models.inputs]]
    name = "image_in"
    datatype = "INT8"
    shape = [1080, 1920, 3]
    id = "inverted_image"

    [[models.outputs]]
    name = "image_out"
    datatype = "INT8"
    shape = [1080, 1920, 3]
    id = "postprocessed_image"
  )"sv;

  const auto toml = toml::parse(toml_str);
  std::cout << toml::json_formatter{toml} << std::endl;
  ModelConfig config{toml};

  ASSERT_NO_FATAL_FAILURE(validateMetadata(config, 0, "preprocess",
                                           "amdinfer_cpp", "base64_decode.so"));
  ASSERT_NO_FATAL_FAILURE(validateMetadata(config, 1, "invert_image",
                                           "amdinfer_cpp", "invert_image.so"));
  ASSERT_NO_FATAL_FAILURE(validateMetadata(config, 2, "postprocess",
                                           "amdinfer_cpp", "base64_encode.so"));

  const auto count = 3;

  ASSERT_EQ(config.size(), count);

  std::array<std::string, count> golden_input_ids{"", "preprocessed_image",
                                                  "inverted_image"};
  std::array<std::string, count> golden_output_ids{
    "preprocessed_image", "inverted_image", "postprocessed_image"};

  for (auto i = 0; i < count; ++i) {
    const auto& inputs = config.inputs(i);
    ASSERT_EQ(inputs.size(), 1);
    const auto& outputs = config.outputs(i);
    ASSERT_EQ(outputs.size(), 1);
    const auto& input = inputs.at(0);
    const auto& output = outputs.at(0);
    Tensor golden_input{"image_in", {1080, 1920, 3}, DataType::Int8};
    ASSERT_NO_FATAL_FAILURE(
      validateTensor(input, golden_input, golden_input_ids.at(i)));
    Tensor golden_output{"image_out", {1080, 1920, 3}, DataType::Int8};
    ASSERT_NO_FATAL_FAILURE(
      validateTensor(output, golden_output, golden_output_ids.at(i)));
  }
}

}  // namespace amdinfer
