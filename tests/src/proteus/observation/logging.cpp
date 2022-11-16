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
 * @brief Implements logging
 */

#include "amdinfer/observation/logging.hpp"

#include <spdlog/sinks/basic_file_sink.h>     // for basic_file_sink_mt, bas...
#include <spdlog/sinks/stdout_color_sinks.h>  // for ansicolor_stdout_sink
#include <spdlog/spdlog.h>

#include <iterator>  // for begin, end
#include <memory>    // for allocator, make_shared
#include <string>    // for string, operator+, char...
#include <vector>    // for vector

namespace amdinfer {

std::string getLogDirectory() { return "."; }

void initLogger() {
  // if already initialized, return early to prevent duplicating logger
  auto logger = spdlog::get("amdinfer");
  if (logger != nullptr) {
    return;
  }

  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::warn);
  console_sink->set_pattern("[amdinfer] [%^%l%$] %v");

  auto dir = getLogDirectory();
  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
    dir + "/amdinfer.log", true);
  file_sink->set_level(spdlog::level::off);

  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(file_sink);
  sinks.push_back(console_sink);

  logger =
    std::make_shared<spdlog::logger>("amdinfer", begin(sinks), end(sinks));
  logger->set_level(spdlog::level::off);
  // logger->flush_on(spdlog::level::info);
  spdlog::register_logger(logger);
  spdlog::set_default_logger(logger);
}

}  // namespace amdinfer
