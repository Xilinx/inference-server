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

#include <string>  // for allocator, basic_string

#include "amdinfer/util/exec.hpp"  // for exec
#include "gtest/gtest.h"           // for Test, SuiteApiResolver, AssertionR...

namespace amdinfer {

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitUtilExec, ShellOneLine) {
  const auto* cmd = "sh -c \"echo 'amdinfer' | sed 's/infer/serve/g'\"";
  auto output = util::exec(cmd);
  EXPECT_EQ(output, "amdserve\n");
}

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitUtilExec, ShellTwoLines) {
  const auto* cmd = "bash -c \"echo -e 'foo\nbar' | sort\"";
  auto output = util::exec(cmd);
  EXPECT_EQ(output, "bar\nfoo\n");
}

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
TEST(UnitUtilExec, NoShell) {
  const auto* cmd = "pwd";
  auto output = util::exec(cmd);
  ASSERT_FALSE(output.empty());
}

}  //  namespace amdinfer
