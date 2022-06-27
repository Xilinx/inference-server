// Copyright 2022 Xilinx Inc.
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
 * @brief Defines the exception classes. Exception classes follow lower-case
 * snake case name syntax of the standard exceptions in std
 */

#ifndef GUARD_PROTEUS_CORE_EXCEPTIONS
#define GUARD_PROTEUS_CORE_EXCEPTIONS

#include <stdexcept>

namespace proteus {

class proteus_error : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class bad_status : public proteus_error {
  using proteus_error::proteus_error;
};

class file_not_found_error : public proteus_error {
  using proteus_error::proteus_error;
};

class file_read_error : public proteus_error {
  using proteus_error::proteus_error;
};

class external_error : public proteus_error {
  using proteus_error::proteus_error;
};

}  // namespace proteus

#endif  // GUARD_PROTEUS_CORE_EXCEPTIONS
