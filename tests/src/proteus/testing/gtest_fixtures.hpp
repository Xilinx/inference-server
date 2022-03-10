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

#ifndef GUARD_SRC_PROTEUS_TESTING_GTEST_FIXTURES
#define GUARD_SRC_PROTEUS_TESTING_GTEST_FIXTURES

#include <proteus/proteus.hpp>

#include "gtest/gtest.h"  // for Test, AssertionResult, EXPECT_EQ

class Grpc : public testing::Test {
 public:
  static void SetUpTestSuite() { proteus::initialize(); };

  static void TearDownTestSuite() { proteus::terminate(); }

 protected:
  void SetUp() override {
    proteus::startGrpcServer(50051);
    client_ = std::make_unique<proteus::GrpcClient>("localhost:50051");
  }

  void TearDown() override { proteus::stopGrpcServer(); }

  std::unique_ptr<proteus::GrpcClient> client_;
};

#endif  // GUARD_SRC_PROTEUS_TESTING_GTEST_FIXTURES
