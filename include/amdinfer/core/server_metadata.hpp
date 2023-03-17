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

#ifndef GUARD_AMDINFER_CORE_SERVER_METADATA
#define GUARD_AMDINFER_CORE_SERVER_METADATA

#include <string>
#include <unordered_set>

namespace amdinfer {

struct ServerMetadata {
  /// Name of the server
  std::string name;
  /// Version of the server
  std::string version;
  /**
   * @brief The extensions supported by the server. The KServe specification
   * allows servers to support custom extensions and return them with a
   * metadata request.
   */
  std::unordered_set<std::string> extensions;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_SERVER_METADATA
