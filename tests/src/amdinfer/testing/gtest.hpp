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

/**
 * @file
 * @brief
 */

#ifndef GUARD_SRC_AMDINFER_TESTING_GTEST
#define GUARD_SRC_AMDINFER_TESTING_GTEST

#include "gtest/gtest.h"  // IWYU pragma: export

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXPECT_THROW_CHECK(statement, check, exception) \
  EXPECT_THROW(                                         \
    {                                                   \
      try {                                             \
        statement                                       \
      } catch (const exception& e) {                    \
        check;                                          \
        throw;                                          \
      }                                                 \
    },                                                  \
    exception)

#endif  // GUARD_SRC_AMDINFER_TESTING_GTEST
