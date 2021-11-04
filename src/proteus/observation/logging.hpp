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

// NDEBUG is defined by Cmake for release builds but could be defined manually
// The log levels are defined in spdlog/common.h
#ifndef PROTEUS_ENABLE_LOGGING
#define SPDLOG_ACTIVE_LEVEL 6
#endif
#ifndef NDEBUG
#ifndef SPDLOG_ACTIVE_LEVEL
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SPDLOG_ACTIVE_LEVEL 2
#endif
#else
#ifndef SPDLOG_ACTIVE_LEVEL
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SPDLOG_ACTIVE_LEVEL 6
#endif
#endif

#include <spdlog/spdlog.h>  // IWYU pragma: export

#include <memory>  // for shared_ptr
#include <string>  // for string

#include "proteus/build_options.hpp"  // for PROTEUS_ENABLE_LOGGING

#ifdef PROTEUS_ENABLE_LOGGING

#if SPDLOG_ACTIVE_LEVEL < SPDLOG_LEVEL_OFF
#define PROTEUS_LOGGING_ACTIVE
#endif

namespace proteus {

/// get the path to the directory to store logs
std::string getLogDirectory();

/// Create and register the loggers used throughout Proteus
void initLogging();

using LoggerPtr = std::shared_ptr<spdlog::logger>;

LoggerPtr getLogger();

}  // namespace proteus
#else
#undef PROTEUS_LOGGING_ACTIVE
#endif

#endif  // GUARD_PROTEUS_OBSERVATION_LOGGING
