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

#include "proteus/testing/get_asset.hpp"

#include <cassert>
#include <fstream>

#include "proteus/core/exceptions.hpp"
#include "proteus/util/string.hpp"

namespace fs = std::filesystem;

namespace proteus {

std::string getAsset(const std::string& key) {
  const auto* root_env = std::getenv("PROTEUS_ROOT");
  if (root_env == nullptr) {
    throw environment_not_set_error("PROTEUS_ROOT not found in the env");
  }
  const auto models_txt = fs::path(root_env) / "external/artifacts/assets.txt";

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
      return substrings[1];
    }
  }
  throw invalid_argument("Key not found in downloaded assets: " + key);
}

}  // namespace proteus
