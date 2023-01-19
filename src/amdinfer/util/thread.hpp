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

#ifndef GUARD_AMDINFER_HELPERS_THREAD
#define GUARD_AMDINFER_HELPERS_THREAD

#include <string>

#ifdef __linux__
#include <sys/prctl.h>
#endif

namespace amdinfer::util {
/**
 * @brief Attempt to set the calling thread's name. Note, this may or may not
 * succeed. If renaming is not possible, it should silently fail without error.
 * There is some discussion on a cross-platform implementation (23899379)
 *
 * @param name Name to set to the thread. If it is >16 bytes, it is silently
 * truncated.
 */
inline void setThreadName(const char* name) {
#ifdef __linux__
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  prctl(PR_SET_NAME, name);
#endif
}

/// string overload for setThreadName
inline void setThreadName(const std::string& name) {
  util::setThreadName(name.c_str());
}

}  // namespace amdinfer::util

#endif  // GUARD_AMDINFER_HELPERS_THREAD
