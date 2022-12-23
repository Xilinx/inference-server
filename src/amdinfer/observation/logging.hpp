// Copyright 2021 Xilinx, Inc.
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

/**
 * @file
 * @brief Defines logging
 */

#ifndef GUARD_AMDINFER_OBSERVATION_LOGGING
#define GUARD_AMDINFER_OBSERVATION_LOGGING

#include <memory>  // for shared_ptr
#include <string>  // for string

#include "amdinfer/build_options.hpp"

#ifdef AMDINFER_ENABLE_LOGGING

#ifndef NDEBUG
// used for debug builds
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#else
// used for release builds
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

#include <spdlog/spdlog.h>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AMDINFER_LOG_TRACE(logger, message) \
  SPDLOG_LOGGER_TRACE((logger).get(), message)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AMDINFER_LOG_DEBUG(logger, message) \
  SPDLOG_LOGGER_DEBUG((logger).get(), message)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AMDINFER_LOG_INFO(logger, message) \
  SPDLOG_LOGGER_INFO((logger).get(), message)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AMDINFER_LOG_WARN(logger, message) \
  SPDLOG_LOGGER_WARN((logger).get(), message)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AMDINFER_LOG_ERROR(logger, message) \
  SPDLOG_LOGGER_ERROR((logger).get(), message)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AMDINFER_IF_LOGGING(args) args
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AMDINFER_LOG_TRACE(logger, message)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AMDINFER_LOG_DEBUG(logger, message)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AMDINFER_LOG_INFO(logger, message)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AMDINFER_LOG_WARN(logger, message)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AMDINFER_LOG_ERROR(logger, message)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define AMDINFER_IF_LOGGING(args)
#endif

namespace amdinfer {

enum class Loggers { Server, Client, Test };

enum class LogLevel {
  Trace,
  Debug,
  Info,
  Warn,
  Error,
  Off,
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
  Logger() = default;
  explicit Logger(Loggers name);

  void set(Loggers name);
  [[nodiscard]] spdlog::logger* get() const { return logger_.get(); }

 private:
  std::shared_ptr<spdlog::logger> logger_;
};

/// Initialize logging for the inference server
void initLogger(const LogOptions& options);

/// get log directory
std::string getLogDirectory();

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_OBSERVATION_LOGGING
