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
 * @brief Defines logging in Proteus
 */

#ifndef GUARD_PROTEUS_OBSERVATION_LOGGING
#define GUARD_PROTEUS_OBSERVATION_LOGGING

#include <memory>  // for shared_ptr
#include <string>  // for string

#include "proteus/build_options.hpp"

namespace spdlog {
class logger;
}

#ifdef PROTEUS_ENABLE_LOGGING
#define PROTEUS_IF_LOGGING(...) __VA_ARGS__
#else
#define PROTEUS_IF_LOGGING(...)
#endif

namespace proteus {

enum class Loggers { kServer, kClient };

enum class LogLevel {
  kTrace,
  kDebug,
  kInfo,
  kWarn,
  kError,
  kOff,
};

struct LogOptions {
  // global options
  std::string logger_name;
  std::string log_directory;

  // file logging
  bool file_enable;
  LogLevel file_level;

  // console logging
  bool console_enable;
  LogLevel console_level;
};

class Logger {
 public:
  explicit Logger(Loggers name);

  void trace(std::string_view message) const;
  void debug(std::string_view message) const;
  void info(std::string_view message) const;
  void warn(std::string_view message) const;
  void error(std::string_view message) const;

 private:
  std::shared_ptr<spdlog::logger> logger_;
};

/// Initialize logging for the inference server
void initLogger(const LogOptions& options);

}  // namespace proteus

#endif  // GUARD_PROTEUS_OBSERVATION_LOGGING
