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

#include "amdinfer/util/filesystem.hpp"

#include <fstream>

#include "amdinfer/core/exceptions.hpp"

namespace fs = std::filesystem;

namespace amdinfer::util {

fs::path findFile(const fs::path& directory, const std::string& extension) {
  if (!fs::exists(directory)) {
    return "";
  }

  fs::path found_path;
  for (const auto& entry : fs::directory_iterator(directory)) {
    if (entry.is_directory()) {
      continue;
    }
    const auto& path = entry.path();
    if (!extension.empty() && path.extension().string() != extension) {
      continue;
    }

    return path;
  }

  return "";
}

std::string readFile(const std::string& path) {
  const size_t block_size = 4096;  // arbitrary value for block size
  std::ifstream stream{path};
  // throw exception in case irrecoverable read error
  stream.exceptions(std::ios_base::badbit);

  std::string out;
  std::string buffer(block_size, '\0');
  while (stream.read(buffer.data(), block_size)) {
    out.append(buffer, 0, stream.gcount());
  }
  out.append(buffer, 0, stream.gcount());
  return out;
}

}  // namespace amdinfer::util
