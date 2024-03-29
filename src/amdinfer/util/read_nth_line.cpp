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

#include "amdinfer/util/read_nth_line.hpp"

#include <fstream>  // for ifstream

namespace amdinfer::util {

std::string readNthLine(const std::string& filename, int n) {
  std::ifstream in(filename);
  std::string line;
  // for performance, reserve some initial space in the string
  const auto default_line_length = 100;
  line.reserve(default_line_length);

  // skip N lines
  for (int i = 0; i < n; ++i) {
    std::getline(in, line);
  }

  std::getline(in, line);
  return line;
}

}  // namespace amdinfer::util
