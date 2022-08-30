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

#include "proteus/util/parse_env.hpp"

#include <cstdlib>  // for getenv
#include <regex>    // for match_results<>::_Base_type, match_results, rege...
#include <string>   // for string, basic_string, allocator
#include <vector>   // for vector

// https://stackoverflow.com/a/23442780

namespace proteus::util {

// Update the input string.
void autoExpandEnvironmentVariables(std::string& text) {
  static std::regex env(R"(\$\{([^}]+)\})");
  std::smatch match;
  while (std::regex_search(text, match, env)) {
    const char* s = getenv(match[1].str().c_str());
    const std::string var(s == nullptr ? "" : s);
    text.replace(match[0].first, match[0].second, var);
  }
}

// Leave input alone and return new string.
std::string expandEnvironmentVariables(const std::string& input) {
  std::string text = input;
  autoExpandEnvironmentVariables(text);
  return text;
}

}  // namespace proteus::util
