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

#include <fcntl.h>                                     // for open, O_RDONLY
#include <google/protobuf/io/zero_copy_stream_impl.h>  // for FileInputStream
#include <google/protobuf/repeated_ptr_field.h>        // for RepeatedPtrField
#include <google/protobuf/text_format.h>               // for TextFormat

#include <chrono>      // for milliseconds
#include <filesystem>  // for path, operator/
#include <thread>      // for sleep_for

#include "model_config.pb.h"                // for Config, InferP...
#include "proteus/core/api.hpp"             // for modelLoad
#include "proteus/core/exceptions.hpp"      // for file_not_found...
#include "proteus/core/manager.hpp"         // for Manager
#include "proteus/core/predict_api.hpp"     // for RequestParameters
#include "proteus/observation/logging.hpp"  // for Logger, PROTEU...

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

void ModelRepository::enableRepositoryMonitoring(bool use_polling) {
  repo_.enableRepositoryMonitoring(use_polling);
}

void ModelRepository::ModelRepositoryImpl::setRepository(
  const std::string& repository_path) {
  repository_ = repository_path;
}

void ModelRepository::ModelRepositoryImpl::modelLoad(
  const std::string& model, RequestParameters* parameters) const {
  const fs::path config_file = "config.pbtxt";

  auto model_path = repository_ / model;
  auto config_path = model_path / config_file;

  // KServe can sometimes create directories like model/model/config_file
  // so if model/config_file doesn't exist, try searching a directory lower too
  if (!fs::exists(config_path) &&
      fs::exists(model_path / model / config_file)) {
    model_path /= model;
    config_path = model_path / config_file;
  }

  // TODO(varunsh): support other versions than 1/
  parameters->put("model", model_path / "1/saved_model");

  inference::Config config;

  int fileDescriptor = open(config_path.c_str(), O_RDONLY);
  Logger logger{Loggers::kServer};
  if (fileDescriptor < 0) {
    throw file_not_found_error("Config file " + config_path.string() +
                               " could not be opened");
  }

  google::protobuf::io::FileInputStream fileInput(fileDescriptor);
  fileInput.SetCloseOnDelete(true);

  if (!google::protobuf::TextFormat::Parse(&fileInput, &config)) {
    throw file_read_error("Config file " + config_path.string() +
                          " could not be parsed");
  }

  if (config.platform() == "tensorflow_graphdef") {
    const auto& inputs = config.inputs();
    // currently supporting one input tensor
    for (const auto& input : inputs) {
      parameters->put("input_node", input.name());
      const auto& shape = input.shape();
      // ZenDNN assumes square image in HWC format
      parameters->put("input_size", static_cast<int>(shape.at(0)));
      parameters->put("image_channels",
                      static_cast<int>(shape.at(shape.size() - 1)));
    }

    const auto& outputs = config.outputs();
    // currently supporting one output tensor
    for (const auto& output : outputs) {
      parameters->put("output_node", output.name());
      const auto& shape = output.shape();
      // ZenDNN assumes [X] classes as output
      parameters->put("output_classes", static_cast<int>(shape.at(0)));
    }

    parameters->put("worker", "tfzendnn");
  } else if (config.platform() == "pytorch_torchscript") {
    parameters->put("worker", "ptzendnn");
  } else if (config.platform() == "onnx_onnxv1") {
    parameters->put("worker", "migraphx");
  } else if (config.platform() == "vitis_xmodel") {
    parameters->put("worker", "xmodel");
  } else {
    throw invalid_argument("Unknown platform: " + config.platform());
  }

  mapProtoToParameters2(config.parameters(), parameters);
}

void UpdateListener::handleFileAction([[maybe_unused]] efsw::WatchID watchid,
                                      const std::string& dir,
                                      const std::string& filename,
                                      efsw::Action action,
                                      std::string oldFilename) {
  Logger logger{Loggers::kServer};
  if (filename == "config.pbtxt") {
    if (action == efsw::Actions::Add) {
      // arbitrary delay to make sure filesystem has settled
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      auto model = fs::path(dir).parent_path().filename();
      // TODO(varunsh): replace with native client
      RequestParameters params;
      try {
        ModelRepository::modelLoad(model, &params);
        Manager::getInstance().loadWorker(model, params);
      } catch (const runtime_error&) {
        PROTEUS_LOG_INFO(logger, "Error loading " + model.string());
      }
    } else if (action == efsw::Actions::Delete) {
      // arbitrary delay to make sure filesystem has settled
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      auto model = fs::path(dir).parent_path().filename();
      // TODO(varunsh): replace with native client
      Manager::getInstance().unloadWorker(model);
    }
  }

  switch (action) {
    case efsw::Actions::Add:
      PROTEUS_LOG_DEBUG(
        logger, "DIR (" + dir + ") FILE (" + filename + ") has event Added");
      break;
    case efsw::Actions::Delete:
      PROTEUS_LOG_DEBUG(
        logger, "DIR (" + dir + ") FILE (" + filename + ") has event Delete");
      break;
    case efsw::Actions::Modified:
      PROTEUS_LOG_DEBUG(
        logger, "DIR (" + dir + ") FILE (" + filename + ") has event Modified");
      break;
    case efsw::Actions::Moved:
      PROTEUS_LOG_DEBUG(logger, "DIR (" + dir + ") FILE (" + filename +
                                  ") has event Moved from (" + oldFilename +
                                  ")");
      break;
    default:
      PROTEUS_LOG_ERROR(logger, "Should never happen");
  }
}

void ModelRepository::ModelRepositoryImpl::enableRepositoryMonitoring(
  bool use_polling) {
  file_watcher_ = std::make_unique<efsw::FileWatcher>(use_polling);
  listener_ = std::make_unique<proteus::UpdateListener>();

  file_watcher_->addWatch(repository_.string(), listener_.get(), true);
  file_watcher_->watch();

  Logger logger{Loggers::kServer};
  for (const auto& path : fs::directory_iterator(repository_)) {
    if (path.is_directory()) {
      auto model = path.path().filename();
      try {
        RequestParameters params;
        proteus::modelLoad(model, &params);
      } catch (const proteus::runtime_error&) {
        PROTEUS_LOG_INFO(logger, "Error loading " + model.string());
      }
    }
  }
}

}  // namespace proteus
