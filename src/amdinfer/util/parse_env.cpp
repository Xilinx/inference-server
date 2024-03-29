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

#include "amdinfer/util/parse_env.hpp"

#include <cstdlib>  // for getenv
#include <iosfwd>   // for _GLIBCXX_USE_CXX11_ABI
#include <regex>    // for match_results<>::_Base_type, match_results, rege...
#include <string>   // for string, basic_string, allocator
#include <vector>   // for vector

namespace amdinfer::util {

// Update the input string.
void autoExpandEnvironmentVariables(std::string& text) {
  // match for ${...} in a string
  static std::regex env{R"(\$\{([^}]+)\})"};
  std::smatch match;
  while (std::regex_search(text, match, env)) {
    // match[1] corresponds to the text inside the {} in the match
    const auto* s = getenv(match[1].str().c_str());
    const std::string var{s == nullptr ? "" : s};
#if _GLIBCXX_USE_CXX11_ABI == 0
    // when compiling without CXX11 ABI, the string and iterator classes behave
    // differently and the original code doesn't compile
    auto start = text.begin();
    std::advance(start, std::distance(text.cbegin(), match[0].first));
    auto end = text.begin();
    std::advance(end, std::distance(text.cbegin(), match[0].second));
    text.replace(start, end, var);
#else
    // first is the iterator to the beginning, second is the iterator to the end
    // of the matched substring
    text.replace(match[0].first, match[0].second, var);
#endif
  }
}

}  // namespace amdinfer::util
