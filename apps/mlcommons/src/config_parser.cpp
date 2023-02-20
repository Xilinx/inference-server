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

/**
 * @file
 * @brief
 */

#include "config_parser.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

#include "amdinfer/core/parameters.hpp"

namespace amdinfer {

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

inline bool startsWith(std::string_view str, std::string_view prefix) {
  return str.size() >= prefix.size() &&
         0 == str.compare(0, prefix.size(), prefix);
}

ParameterMap Config::getParameters(const std::string& model,
                                   const std::string& scenario) {
  ParameterMap parameters = config_;

  for (const auto& [key, value] : config_) {
    auto split_key = split(key, ".");
    if (split_key[2] != "parameters") {
      parameters.erase(key);
      continue;
    }

    if (split_key[0] == model && split_key[1] == scenario) {
      parameters.rename(key, split_key[3]);
    } else if (split_key[0] == "*" && split_key[1] == scenario) {
      parameters.rename(key, split_key[3]);
    } else if (split_key[0] == model && split_key[1] == "*") {
      parameters.rename(key, split_key[3]);
    } else if (split_key[0] == "*" && split_key[1] == "*") {
      parameters.rename(key, split_key[3]);
    } else {
      throw invalid_argument("bad key value stored");
    }
  }

  return parameters;
}

Config parseConfig(const std::string& path) {
  // assuming entry is "key = value" so splitting by space yields 3 values
  const auto num_substrings = 3;
  // assuming label for parameters is amdinfer.<model>.<scenario>.<name>.<type>
  // so splitting by space yields 5 values
  const auto num_substrings_custom = 5;
  // assuming label for parameters is
  // amdinfer.<model>.<scenario>.parameters.<name>.<type> so splitting by space
  // yields 6 values
  const auto num_substrings_parameters = 6;

  Config config;

  std::ifstream file{path};
  std::string line;
  while (std::getline(file, line)) {
    // skip comments
    if (line.empty() || line.at(0) == '#') {
      continue;
    }

    auto substrings = split(line, " ");
    if (substrings.size() != num_substrings) {
      std::cerr << "Config parsing error: skipping line " << line << std::endl;
      continue;
    }

    const auto& key = substrings[0];
    const auto& value = substrings[2];
    auto split_key = split(key, ".");
    if (split_key.size() < num_substrings) {
      std::cerr << "Config parsing error: skipping line " << line << std::endl;
      continue;
    }

    // skip all the non-custom keys assuming loadgen will parse them
    if (split_key[0] != "amdinfer") {
      continue;
    }

    std::string parameter_key;
    std::string type;
    if (split_key[3] == "parameters") {
      if (split_key.size() != num_substrings_parameters) {
        std::cerr << "Config parsing error: skipping line " << line
                  << std::endl;
        continue;
      }
      parameter_key = split_key[1] + "." + split_key[2] + "." + split_key[3] +
                      "." + split_key[4];
      type = split_key[num_substrings_parameters - 1];
    } else {
      if (split_key.size() != num_substrings_custom) {
        std::cerr << "Config parsing error: skipping line " << line
                  << std::endl;
        continue;
      }
      parameter_key = split_key[1] + "." + split_key[2] + "." + split_key[3];
      type = split_key[num_substrings_custom - 1];
    }
    if (type == "int") {
      config.put(parameter_key, std::stoi(value));
    } else if (type == "double") {
      config.put(parameter_key, std::stod(value));
    } else if (type == "bool") {
      try {
        config.put(parameter_key, static_cast<bool>(std::stoi(value)));
      } catch (const std::invalid_argument& e) {
        std::cerr << "Config parsing error: could not convert string to int: "
                  << value << "\n";
        continue;
      }

    } else {
      config.put(parameter_key, value);
    }
  }

  return config;
}

}  // namespace amdinfer
