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

#include <cstdlib>  // for getenv, EXIT_SUCCESS
#include <iostream>

#include "gtest/gtest.h"  // for Test, AssertionResult, SuiteApiRe...
#include "xmodel.hpp"

std::string xmodel = std::string(std::getenv("AKS_XMODEL_ROOT")) +
                     "/artifacts/u200_u250/densebox_320_320/"
                     "densebox_320_320.xmodel";
int images = 100;
int threads = 1;
int runners = 1;
std::string path = std::string(std::getenv("PROTEUS_ROOT")) + "/tests/assets";

class XModelFixture : public testing::Test {
 public:
  static void SetUpTestSuite() { proteus::initialize(); };

  static void TearDownTestSuite() { proteus::terminate(); }
};

// @pytest.mark.extensions(["vitis"])
// @pytest.mark.fpgas("DPUCADF8H", 2)
TEST_F(XModelFixture, proteus) {
  auto fpgas_exist = proteus::hasHardware("DPUCADF8H", 1);
  if (!fpgas_exist) {
    GTEST_SKIP();
  }
  EXPECT_TRUE(run(xmodel, images, threads, runners) == EXIT_SUCCESS);
}

// @pytest.mark.extensions(["vitis"])
// @pytest.mark.fpgas("DPUCADF8H", 2)
TEST_F(XModelFixture, reference) {
  auto fpgas_exist = proteus::hasHardware("DPUCADF8H", 1);
  if (!fpgas_exist) {
    GTEST_SKIP();
  }
  EXPECT_TRUE(run_reference(xmodel, images, threads, runners) == EXIT_SUCCESS);
}
