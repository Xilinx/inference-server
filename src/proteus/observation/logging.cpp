// Copyright 2021 Xilinx Inc.
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

/**
 * @file
 * @brief Implements logging in Proteus
 */

#include "proteus/observation/logging.hpp"

#include <spdlog/sinks/basic_file_sink.h>     // for basic_file_sink_mt, bas...
#include <spdlog/sinks/stdout_color_sinks.h>  // for ansicolor_stdout_sink
#include <spdlog/spdlog.h>

#include <cassert>   // for assert
#include <cstdlib>   // for getenv
#include <iterator>  // for begin, end
#include <memory>    // for allocator, make_shared
#include <string>    // for string, operator+, char...
#include <vector>    // for vector

namespace proteus {

constexpr spdlog::level::level_enum getLevel(LogLevel level) {
  switch (level) {
    case LogLevel::kTrace:
      return spdlog::level::trace;
    case LogLevel::kDebug:
      return spdlog::level::debug;
    case LogLevel::kInfo:
      return spdlog::level::info;
    case LogLevel::kWarn:
      return spdlog::level::warn;
    case LogLevel::kError:
      return spdlog::level::err;
    default:
      return spdlog::level::off;
  }
}

void initLogger(const LogOptions& options) {
  // if already initialized, return early to prevent duplicating logger
  auto logger = spdlog::get(options.logger_name);
  if (logger != nullptr) {
    return;
  }

  // at least one must be enabled though may be configured off by the log level
  assert(options.console_enable || options.file_enable);

  std::vector<spdlog::sink_ptr> sinks;

  if (options.console_enable) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(getLevel(options.console_level));
    console_sink->set_pattern("[proteus] [%^%l%$] %v");
    sinks.push_back(console_sink);
  }

  if (options.file_enable) {
    auto log_file = options.log_directory + "/" + options.logger_name + ".log";
    auto file_sink =
      std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);
    file_sink->set_level(getLevel(options.file_level));
    sinks.push_back(file_sink);
  }

  logger = std::make_shared<spdlog::logger>(options.logger_name,
                                            std::begin(sinks), std::end(sinks));
  logger->set_level(spdlog::level::trace);
  logger->flush_on(spdlog::level::info);
  spdlog::register_logger(logger);
}

Logger::Logger(Loggers name) {
  if (name == Loggers::kClient) {
    logger_ = spdlog::get("client");
  } else {
    logger_ = spdlog::get("server");
  }
  assert(logger_ != nullptr);
}

std::string getLogDirectory() {
  auto* home = std::getenv("HOME");
  std::string dir;
  if (home != nullptr) {
    dir = home;
    dir += "/.proteus";
  } else {
    dir = ".";
  }
  dir += "/logs";
  return dir;
}

}  // namespace proteus
