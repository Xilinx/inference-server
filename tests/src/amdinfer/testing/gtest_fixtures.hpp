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

#ifndef GUARD_SRC_AMDINFER_TESTING_GTEST_FIXTURES
#define GUARD_SRC_AMDINFER_TESTING_GTEST_FIXTURES

#include <amdinfer/amdinfer.hpp>
#include <memory>  // for allocator, unique_ptr

#include "gtest/gtest.h"  // IWYU pragma: export

class BaseFixture : public testing::Test {
 public:
  // because of the session scoped HTTP fixture, the server needs to be static
  // NOLINTNEXTLINE(cert-err58-cpp)
  inline static amdinfer::Server server_;

 protected:
  void SetUp() override {
    const auto* root = std::getenv("AMDINFER_ROOT");
    if (root == nullptr) {
      throw amdinfer::environment_not_set_error("AMDINFER_ROOT is not set");
    }
    server_.setModelRepository(
      std::string{root} + "/external/artifacts/repository", false);
  }
};

template <typename T>
class BaseFixtureWithParams : public BaseFixture,
                              public testing::WithParamInterface<T> {};

class GrpcFixture : public BaseFixture {
 protected:
  void SetUp() override {
    client_ = std::make_unique<amdinfer::GrpcClient>(
      "localhost:" + std::to_string(kDefaultGrpcPort));
    if (!client_->serverLive()) {
      BaseFixture::SetUp();
      server_.startGrpc(kDefaultGrpcPort);
      started_ = true;
      while (!client_->serverLive()) {
        std::this_thread::yield();
      }
    }
  }

  std::unique_ptr<amdinfer::GrpcClient> client_;
  bool started_ = false;
};

template <typename T>
class GrpcFixtureWithParams : public GrpcFixture,
                              public testing::WithParamInterface<T> {};

class HttpFixture : public BaseFixture {
 protected:
  // Drogon is using a singleton that doesn't work well being restarted so set
  // it up once per suite. This also means we can't have multiple test suites in
  // the same executable that use HTTP
  static void SetUpTestSuite() {
    // void SetUp() override {
    client_ = std::make_unique<amdinfer::HttpClient>("http://127.0.0.1:8998");
    if (!client_->serverLive()) {
      server_.startHttp(kDefaultHttpPort);
      started_ = true;
      while (!client_->serverLive()) {
        std::this_thread::yield();
      }
    }
  }

  static void TearDownTestSuite() {
    // void TearDown() override {
    if (started_) {
      server_.stopHttp();
    }
    client_.reset(nullptr);
  }

  inline static std::unique_ptr<amdinfer::HttpClient> client_;
  inline static bool started_ = false;
  // std::unique_ptr<amdinfer::HttpClient> client_;
  // bool started_ = false;
};

template <typename T>
class HttpFixtureWithParams : public HttpFixture,
                              public testing::WithParamInterface<T> {};

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

#endif  // GUARD_SRC_AMDINFER_TESTING_GTEST_FIXTURES
