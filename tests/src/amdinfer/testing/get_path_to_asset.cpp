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

#include "amdinfer/testing/get_path_to_asset.hpp"

#include <cassert>             // for assert
#include <cstdlib>             // for getenv
#include <ext/alloc_traits.h>  // for __alloc_traits<>::value_type
#include <filesystem>          // for path, operator/, filesystem
#include <fstream>             // for ifstream, basic_istream
#include <memory>              // for allocator_traits<>::value_type
#include <vector>              // for vector

#include "amdinfer/core/exceptions.hpp"  // for invalid_argument, environmen...
#include "amdinfer/util/string.hpp"      // for split, startsWith

namespace fs = std::filesystem;

namespace amdinfer {

std::string getPathToAsset(const std::string& key) {
  const auto* root_env = std::getenv("AMDINFER_ROOT");
  if (root_env == nullptr) {
    throw environment_not_set_error("AMDINFER_ROOT not found in the env");
  }
  const fs::path root_path{root_env};
  const auto models_txt = root_path / "tests/assets/artifacts.txt";

  std::ifstream models_file;
  models_file.open(models_txt);

  if (!models_file.is_open()) {
    throw file_not_found_error("Cannot open file " + models_txt.string());
  }

  std::string line;
  while (getline(models_file, line)) {
    if (util::startsWith(line, key)) {
      auto substrings = util::split(line, ":");
      assert(substrings.size() == 2);
      const auto& path = substrings[1];
      if (path.empty()) {
        throw invalid_argument("No path found for key: " + key);
      }
      // if the path is absolute, return it as is. Otherwise, prefix it with
      // the path to the repository
      if (util::startsWith(path, "/")) {
        return path;
      }
      return root_path / substrings[1];
    }
  }
  throw invalid_argument("Key not found in downloaded assets: " + key);
}

}  // namespace amdinfer
