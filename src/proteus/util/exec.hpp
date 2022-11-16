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

#ifndef GUARD_AMDINFER_UTIL_EXEC
#define GUARD_AMDINFER_UTIL_EXEC

#include <string>

namespace amdinfer::util {

/**
 * @brief Execute an arbitrary command on the command line.
 * Taken from https://stackoverflow.com/a/478960
 *
 * @param cmd command to run
 * @return std::string output of the command (stdout)
 */
std::string exec(const char* cmd);

}  // namespace amdinfer::util

#endif  // GUARD_AMDINFER_UTIL_EXEC
