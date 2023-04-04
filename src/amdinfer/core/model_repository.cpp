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

#include "amdinfer/core/model_repository.hpp"

#include <fcntl.h>                                     // for open, O_CLOEXEC
#include <google/protobuf/io/zero_copy_stream_impl.h>  // for FileInputStream
#include <google/protobuf/repeated_ptr_field.h>        // for RepeatedPtrField
#include <google/protobuf/text_format.h>               // for TextFormat

#include <chrono>      // for milliseconds
#include <filesystem>  // for path, operator/
#include <thread>      // for sleep_for

#include "amdinfer/core/endpoints.hpp"       // for Endpoints
#include "amdinfer/core/exceptions.hpp"      // for runtime_error
#include "amdinfer/core/model_config.hpp"    // for ModelConfig
#include "amdinfer/core/parameters.hpp"      // for ParameterMap
#include "amdinfer/observation/logging.hpp"  // for AMDINFER_LOG_D...
#include "amdinfer/util/filesystem.hpp"      // for findFile
#include "model_config.pb.h"                 // for Config, InferP...

namespace fs = std::filesystem;

namespace amdinfer {

// TODO(varunsh): get rid of this duplicate code with the one in grpc_internal
void mapProtoToParameters2(
  const google::protobuf::Map<std::string, inference::InferParameter2>& params,
  ParameterMap* parameters) {
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

void parseModel(const fs::path& repository, const std::string& model,
                ParameterMap* parameters) {
  auto model_path = repository / model;

  fs::path config_path;
  try {
    config_path = util::findFile(model_path, ".pbtxt");
  } catch (const file_not_found_error&) {
    // this is okay, try again at a different path as well
  }

  // KServe can sometimes create directories like model/model/config_file
  // so if model/config_file doesn't exist, try searching a directory lower too
  if (config_path.empty()) {
    model_path /= model;
    // if this fails, let it throw
    config_path = util::findFile(model_path / model, ".pbtxt");
  }

  inference::Config proto_config;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  int file_descriptor = open(config_path.c_str(), O_RDONLY | O_CLOEXEC);
  AMDINFER_IF_LOGGING(Logger logger{Loggers::Server};)
  if (file_descriptor < 0) {
    throw file_not_found_error("Config file " + config_path.string() +
                               " could not be opened");
  }

  google::protobuf::io::FileInputStream file_input(file_descriptor);
  file_input.SetCloseOnDelete(true);

  if (!google::protobuf::TextFormat::Parse(&file_input, &proto_config)) {
    throw file_read_error("Config file " + config_path.string() +
                          " could not be parsed");
  }

  // TODO(varunsh): support other versions than 1/
  const auto model_base = model_path / "1";

  ModelConfig config{proto_config};

  if (config.platform() == "tensorflow_graphdef") {
    const auto& inputs = config.inputs();
    if (inputs.size() != 1) {
      throw invalid_argument("Currently, there must be one input tensor");
    }
    const auto& input = inputs.at(0);
    parameters->put("input_node", input.getName());
    const auto& input_shape = input.getShape();
    // ZenDNN assumes square image in HWC format
    parameters->put("input_size", static_cast<int>(input_shape.at(0)));
    parameters->put("image_channels",
                    static_cast<int>(input_shape.at(input_shape.size() - 1)));

    const auto& outputs = config.outputs();
    if (outputs.size() != 1) {
      throw invalid_argument("Currently, there must be one output tensor");
    }
    const auto& output = outputs.at(0);
    parameters->put("output_node", output.getName());
    const auto& output_shape = output.getShape();
    // ZenDNN assumes [X] classes as output
    parameters->put("output_classes", static_cast<int>(output_shape.at(0)));

    parameters->put("worker", "tfzendnn");
    const auto model_file = util::findFile(model_base, ".pb");
    parameters->put("model", model_file.string());
  } else if (config.platform() == "pytorch_torchscript") {
    parameters->put("worker", "ptzendnn");
    const auto model_file = util::findFile(model_base, ".pt");
    parameters->put("model", model_file.string());
  } else if (config.platform() == "onnx_onnxv1") {
    parameters->put("worker", "migraphx");
    const auto model_file = util::findFile(model_base, ".onnx");
    parameters->put("model", model_file.string());
  } else if (config.platform() == "migraphx_mxr") {
    parameters->put("worker", "migraphx");
    const auto model_file = util::findFile(model_base, ".mxr");
    parameters->put("model", model_file.string());
  } else if (config.platform() == "vitis_xmodel") {
    parameters->put("worker", "xmodel");
    const auto model_file = util::findFile(model_base, ".xmodel");
    parameters->put("model", model_file.string());
  } else {
    throw invalid_argument("Unknown platform: " + config.platform());
  }

  mapProtoToParameters2(proto_config.parameters(), parameters);
}

void ModelRepository::setRepository(const fs::path& repository_path,
                                    bool load_existing) {
  repository_ = repository_path;
  if (fs::exists(repository_path) && load_existing) {
    AMDINFER_IF_LOGGING(Logger logger{Loggers::Server};)
    for (const auto& path : fs::directory_iterator(repository_)) {
      if (path.is_directory()) {
        auto model = path.path().filename();
        try {
          ParameterMap params;
          endpoints_->load(model, params);
        } catch (const amdinfer::runtime_error& e) {
          AMDINFER_LOG_INFO(
            logger, "Error loading " + model.string() + ": " + e.what());
        }
      }
    }
  }
}

std::string ModelRepository::getRepository() const {
  return repository_.string();
}

void ModelRepository::enableMonitoring(bool use_polling) {
  file_watcher_ = std::make_unique<efsw::FileWatcher>(use_polling);
  listener_ =
    std::make_unique<amdinfer::UpdateListener>(repository_, endpoints_);

  file_watcher_->addWatch(repository_.string(), listener_.get(), true);
  file_watcher_->watch();
}

void UpdateListener::handleFileAction(
  [[maybe_unused]] efsw::WatchID watch_id, const std::string& dir,
  const std::string& filename, efsw::Action action,
  [[maybe_unused]] std::string old_filename) {
  AMDINFER_IF_LOGGING(Logger logger{Loggers::Server};)
  // arbitrary delay to make sure filesystem has settled
  const std::chrono::milliseconds delay{100};
  if (filename == "proto_config.pbtxt") {
    if (action == efsw::Actions::Add) {
      std::this_thread::sleep_for(delay);
      auto model = fs::path(dir).parent_path().filename();
      ParameterMap params;
      try {
        parseModel(repository_, model, &params);
        endpoints_->load(model, params);
      } catch (const runtime_error&) {
        AMDINFER_LOG_INFO(logger, "Error loading " + model.string());
      }
    } else if (action == efsw::Actions::Delete) {
      // arbitrary delay to make sure filesystem has settled
      std::this_thread::sleep_for(delay);
      auto model = fs::path(dir).parent_path().filename();
      endpoints_->unload(model.string());
    }
  }

  switch (action) {
    case efsw::Actions::Add:
      AMDINFER_LOG_DEBUG(
        logger, "DIR (" + dir + ") FILE (" + filename + ") has event Added");
      break;
    case efsw::Actions::Delete:
      AMDINFER_LOG_DEBUG(
        logger, "DIR (" + dir + ") FILE (" + filename + ") has event Delete");
      break;
    case efsw::Actions::Modified:
      AMDINFER_LOG_DEBUG(
        logger, "DIR (" + dir + ") FILE (" + filename + ") has event Modified");
      break;
    case efsw::Actions::Moved:
      AMDINFER_LOG_DEBUG(logger, "DIR (" + dir + ") FILE (" + filename +
                                   ") has event Moved from (" + old_filename +
                                   ")");
      break;
    default:
      AMDINFER_LOG_ERROR(logger, "Should never happen");
  }
}

void ModelRepository::setEndpoints(Endpoints* endpoints) {
  endpoints_ = endpoints;
}

}  // namespace amdinfer
