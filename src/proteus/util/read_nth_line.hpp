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

#ifndef GUARD_PROTEUS_UTIL_READ_NTH_LINE
#define GUARD_PROTEUS_UTIL_READ_NTH_LINE

#include <fstream>
#include <string>

namespace proteus::util {

/**
 * @brief This method will read the class file and returns class name
 *
 * @param filename: The location of the file containing class names
 * @param N: Nth value from the class file
 * @return std::string: Name of the class
 */
std::string readNthLine(const std::string& filename, int N) {
  std::ifstream in(filename);
  std::string line;
  // for performance, reserve some initial space in the string
  const auto kDefaultLineLength = 100;
  line.reserve(kDefaultLineLength);

  // skip N lines
  for (int i = 0; i < N; ++i) {
    std::getline(in, line);
  }

  std::getline(in, line);
  return line;
}

}  // namespace proteus::util

#endif  // GUARD_PROTEUS_UTIL_READ_NTH_LINE
