// Copyright 2022 Xilinx Inc.
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

#include "proteus/core/model_repository.hpp"

#include <fcntl.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include <cassert>
#include <filesystem>
#include <fstream>

#include "model_config.pb.h"
#include "proteus/core/predict_api.hpp"

namespace fs = std::filesystem;

namespace proteus {

ModelRepository::ModelRepository(const std::string& repository_path)
  : repository_(repository_path) {}

void ModelRepository::modelLoad(const std::string& model,
                                RequestParameters* parameters) const {
  const auto model_path = repository_ / model;
  const auto config_path = model_path / "config.pbtxt";

  // TODO(varunsh): support other versions than 1/
  parameters->put("model", model_path / "1/saved_model");

  inference::Config config;

  int fileDescriptor = open(config_path.c_str(), O_RDONLY);

  if (fileDescriptor < 0) {
    throw std::runtime_error("Config file could not be opened");
  }

  google::protobuf::io::FileInputStream fileInput(fileDescriptor);
  fileInput.SetCloseOnDelete(true);

  if (!google::protobuf::TextFormat::Parse(&fileInput, &config)) {
    throw std::runtime_error("Config file could not be parsed");
  }

  if (const auto& platform = config.platform();
      platform == "tensorflow_graphdef") {
    parameters->put("worker", "tfzendnn");
  } else if (platform == "vitis_xmodel") {
    parameters->put("worker", "xmodel");
  } else {
    throw std::runtime_error("Unknown platform");
  }
}

}  // namespace proteus
