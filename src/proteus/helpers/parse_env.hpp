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

#ifndef GUARD_PROTEUS_HELPERS_PARSE_ENV
#define GUARD_PROTEUS_HELPERS_PARSE_ENV

#include <string>

namespace proteus {

/// Expand any environment variables in the string in place
void autoExpandEnvironmentVariables(std::string& text);

// Expand any environment variables in the string and return a copy
std::string expandEnvironmentVariables(const std::string& input);

}  // namespace proteus

#endif  // GUARD_PROTEUS_HELPERS_PARSE_ENV
