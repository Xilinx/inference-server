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

#include "amdinfer/core/model_config.hpp"        // for ModelConfig
#include "amdinfer/core/versioned_endpoint.hpp"  // for getVersionedEndpoint
#include "gtest/gtest.h"  // for Message, TestPartResult, Test

using namespace std::string_view_literals;

namespace amdinfer {

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
  ModelConfig config{toml, ""};

  ASSERT_EQ(config.size(), 1);
  for (const auto& [model, parameters] : config) {
    ASSERT_EQ(model, "mnist");
    ASSERT_EQ(parameters.get<std::string>("worker"), "tfzendnn");
  }
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
  ModelConfig config{toml, ""};

  ASSERT_EQ(config.size(), 1);
  for (const auto& [model, parameters] : config) {
    ASSERT_EQ(model, "mnist");
    ASSERT_EQ(parameters.get<std::string>("worker"), "tfzendnn");
  }
}

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitModelConfig, Chain) {
  constexpr std::string_view toml_str = R"(
    [[models]]
    name = "invert_image"
    platform = "amdinfer_cpp"
    id = "base64_decode.so"

    [[models.inputs]]
    name = "image_in"
    datatype = "BYTES"
    shape = [1048576]
    id = ""

    [[models.outputs]]
    name = "image_out"
    datatype = "INT8"
    shape = [1080, 1920, 3]
    id = "preprocessed_image"

    [[models]]
    name = "execute"
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
    name = "invert_image_postprocess"
    platform = "amdinfer_cpp"
    id = "base64_encode.so"

    [[models.inputs]]
    name = "image_in"
    datatype = "INT8"
    shape = [1080, 1920, 3]
    id = "inverted_image"

    [[models.outputs]]
    name = "image_out"
    datatype = "BYTES"
    shape = [1048576]
    id = "postprocessed_image"
  )"sv;

  const auto toml = toml::parse(toml_str);
  const std::string version{"1"};
  ModelConfig config{toml, version};

  ASSERT_EQ(config.size(), 3);
  std::array<std::string, 3> models{"invert_image", "execute",
                                    "invert_image_postprocess"};

  auto index = 0;
  for (const auto& [model, parameters] : config) {
    ASSERT_EQ(model, getVersionedEndpoint(models.at(index), version));
    ASSERT_EQ(parameters.get<std::string>("worker"), "cplusplus");
    index++;
  }
}

}  // namespace amdinfer
