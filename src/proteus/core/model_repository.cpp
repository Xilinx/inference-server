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

// TODO(varunsh): get rid of this duplicate code with the one in grpc_internal
void mapProtoToParameters2(
  const google::protobuf::Map<std::string, inference::InferParameter2>& params,
  RequestParameters* parameters) {
  using ParameterType = inference::InferParameter2::ParameterChoiceCase;
  for (const auto& [key, value] : params) {
    auto type = value.parameter_choice_case();
    switch (type) {
      case ParameterType::kBoolParam: {
        parameters->put(key, value.bool_param());
        break;
      }
      case ParameterType::kInt64Param: {
        // TODO(varunsh): parameters should switch to uint64?
        parameters->put(key, static_cast<int>(value.int64_param()));
        break;
      }
      case ParameterType::kDoubleParam: {
        parameters->put(key, value.double_param());
        break;
      }
      case ParameterType::kStringParam: {
        parameters->put(key, value.string_param());
        break;
      }
      default: {
        // if not set
        break;
      }
    }
  }
}

void ModelRepository::modelLoad(const std::string& model,
                                RequestParameters* parameters) {
  repo_.modelLoad(model, parameters);
}

void ModelRepository::setRepository(const std::string& repository) {
  repo_.setRepository(repository);
}

void ModelRepository::ModelRepositoryImpl::setRepository(
  const std::string& repository_path) {
  repository_ = repository_path;
}

void ModelRepository::ModelRepositoryImpl::modelLoad(
  const std::string& model, RequestParameters* parameters) const {
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

  mapProtoToParameters2(config.parameters(), parameters);
}

}  // namespace proteus
