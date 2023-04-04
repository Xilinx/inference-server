// Copyright 2023 Advanced Micro Devices, Inc.
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

#ifndef GUARD_AMDINFER_UTIL_FILESYSTEM
#define GUARD_AMDINFER_UTIL_FILESYSTEM

#include <filesystem>

namespace amdinfer::util {

/**
 * @brief Get the path to the file in a directory with the given extension. If
 * there is more than one file in the directory that matches, only the first one
 * found is returned. Subdirectories are not searched.
 *
 * @param directory path to search in
 * @param extension extension of the file to search for
 * @return std::filesystem::path
 */
std::filesystem::path findFile(const std::filesystem::path& directory,
                               const std::string& extension);

}  // namespace amdinfer::util

#endif  // GUARD_AMDINFER_UTIL_FILESYSTEM
