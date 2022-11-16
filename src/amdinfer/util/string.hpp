// Copyright 2022 Xilinx, Inc.
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

#ifndef GUARD_AMDINFER_HELPERS_STRING
#define GUARD_AMDINFER_HELPERS_STRING

#include <algorithm>
#include <sstream>
#include <string>

namespace amdinfer::util {

/**
 * @brief Checks if a string ends with another string
 *
 * @param str full string
 * @param suffix suffix to check
 * @return bool true if str ends with suffix
 */
inline bool endsWith(std::string_view str, std::string_view suffix) {
  return str.size() >= suffix.size() &&
         0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

/**
 * @brief Checks if a string starts with another string
 *
 * @param str full string
 * @param suffix prefix to check
 * @return bool true if str starts with prefix
 */
inline bool startsWith(std::string_view str, std::string_view prefix) {
  return str.size() >= prefix.size() &&
         0 == str.compare(0, prefix.size(), prefix);
}

inline std::vector<std::string> split(std::string_view str,
                                      std::string_view delimiter) {
  auto last = 0UL;
  auto next = 0UL;
  std::vector<std::string> substrings;
  while ((next = str.find(delimiter, last)) != std::string::npos) {
    substrings.emplace_back(str.substr(last, next - last));
    last = next + 1;
  }
  substrings.emplace_back(str.substr(last));
  return substrings;
}

inline std::string toLower(const std::string& str) {
  auto str_lower = str;
  std::transform(str_lower.begin(), str_lower.end(), str_lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return str_lower;
}

inline void toLower(std::string* str) {
  std::transform(str->begin(), str->end(), str->begin(),
                 [](unsigned char c) { return std::tolower(c); });
}

inline std::string addressToString(const void* ptr) {
  std::ostringstream addr;
  addr << ptr;
  return addr.str();
}

}  // namespace amdinfer::util

#endif  // GUARD_AMDINFER_HELPERS_STRING
