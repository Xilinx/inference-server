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
#include <toml++/toml.h>

#include <chrono>      // for milliseconds
#include <filesystem>  // for path, operator/
#include <thread>      // for sleep_for

#include "amdinfer/core/endpoints.hpp"       // for Endpoints
#include "amdinfer/core/exceptions.hpp"      // for runtime_error
#include "amdinfer/core/model_config.hpp"    // for ModelConfig
#include "amdinfer/core/parameters.hpp"      // for ParameterMap
#include "amdinfer/observation/logging.hpp"  // for AMDINFER_LOG_D...
#include "amdinfer/util/filesystem.hpp"      // for findFile
#include "amdinfer/util/string.hpp"          // for endsWith
#include "model_config.hpp"                  // for ModelConfig
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

fs::path findConfigFile(const fs::path& model_path, const std::string& model) {
  std::array extensions{".toml", ".pbtxt"};
  for (const auto* extension : extensions) {
    auto config_path = util::findFile(model_path, extension);
    if (!config_path.empty()) {
      return config_path;
    }

    // KServe can sometimes create directories like model/model/config_file
    // so if model/config_file doesn't exist, try searching a directory lower
    // too
    config_path = util::findFile(model_path / model, extension);
    if (!config_path.empty()) {
      return config_path;
    }
  }

  throw file_not_found_error("No configuration file could be loaded in " +
                             model_path.string());
}

ModelConfig openConfigFile(const fs::path& config_path,
                           const std::string& version) {
  if (config_path.extension() == ".pbtxt") {
    inference::Config proto_config;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    int file_descriptor = open(config_path.c_str(), O_RDONLY | O_CLOEXEC);
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

    return ModelConfig{proto_config, version};
  }
  if (config_path.extension() == ".toml") {
    auto toml = toml::parse_file(config_path.string());

    return ModelConfig{toml, version};
  }
  throw invalid_argument(
    "Unknown config file extension passed to openConfigFile: " +
    config_path.extension().string());
}

ModelConfig parseModel(const fs::path& repository, const std::string& model,
                       const std::string& version) {
  AMDINFER_IF_LOGGING(Logger logger{Loggers::Server};)
  auto model_path = repository / model;

  auto config_path = findConfigFile(model_path, model);
  auto config = openConfigFile(config_path, version);

  const auto assigned_version = version.empty() ? "1" : version;
  const auto model_base = config_path.parent_path() / assigned_version;

  config.setModelFiles(model_base);
  return config;
}

void loadModel(const fs::path& repository, const fs::path& model_name,
               Endpoints* endpoints) {
  auto config = parseModel(repository, model_name, "");
  for (const auto& [model, parameters] : config) {
    endpoints->load(model, "", parameters);
  }
}

void ModelRepository::setRepository(const fs::path& repository_path,
                                    bool load_existing) {
  repository_ = repository_path;
  if (fs::exists(repository_path) && load_existing) {
    AMDINFER_IF_LOGGING(Logger logger{Loggers::Server};)
    for (const auto& path : fs::directory_iterator(repository_)) {
      if (path.is_directory()) {
        auto model_name = path.path().filename();
        try {
          loadModel(repository_, model_name, endpoints_);
        } catch (const amdinfer::runtime_error& e) {
          AMDINFER_LOG_INFO(
            logger, "Error loading " + model_name.string() + ": " + e.what());
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
  if (filename == "config.pbtxt" || util::endsWith(filename, "toml")) {
    if (action == efsw::Actions::Add) {
      std::this_thread::sleep_for(delay);
      auto model_name = fs::path(dir).parent_path().filename();
      try {
        loadModel(repository_, model_name, endpoints_);
      } catch (const runtime_error&) {
        AMDINFER_LOG_INFO(logger, "Error loading " + model_name.string());
      }
    } else if (action == efsw::Actions::Delete) {
      // arbitrary delay to make sure filesystem has settled
      std::this_thread::sleep_for(delay);
      auto model = fs::path(dir).parent_path().filename();
      endpoints_->unload(model.string(), "");
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
