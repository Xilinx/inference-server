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

#include <cstddef>   // for size_t
#include <cstdint>   // for uint8_t, uint64_t
#include <memory>    // for allocator, make_unique
#include <optional>  // for optional
#include <utility>   // for pair, move
#include <vector>    // for vector

#include "gtest/gtest.h"                        // for EXPECT_EQ, UnitTest
#include "proteus/batching/soft.hpp"            // for BatchPtr, SoftBatcher
#include "proteus/buffers/buffer.hpp"           // for Buffer
#include "proteus/buffers/vector_buffer.hpp"    // for VectorBuffer
#include "proteus/build_options.hpp"            // for PROTEUS_ENABLE_LOGGING
#include "proteus/clients/native_internal.hpp"  // for CppNativeApi
#include "proteus/core/data_types.hpp"          // for DataType, DataType::U...
#include "proteus/core/predict_api.hpp"         // for InferenceRequest, Req...
#include "proteus/core/worker_info.hpp"         // for WorkerInfo
#include "proteus/helpers/declarations.hpp"     // for BufferPtrs

namespace proteus {

constexpr auto kTimeoutMs = 1000;  // timeout in ms
constexpr auto kTimeoutUs =
  kTimeoutMs * 1000 * 1.5;  // timeout in us with buffer

class UnitSoftBatcherFixture
  : public testing::TestWithParam<std::pair<int, int>> {
 protected:
  void SetUp() override {
#ifdef PROTEUS_ENABLE_LOGGING
    LogOptions options{
      "server",          // logger_name
      "",                // log directory
      false,             // enable file logging
      LogLevel::kDebug,  // file log level
      true,              // enable console logging
      LogLevel::kWarn    // console log level
    };
    initLogger(options);
#endif

    auto [batch_size, requests] = GetParam();

    const auto kBufferNum = 10;
    const auto kDataShape = {1UL, 2UL, 50UL};
    const auto kDataSize = 100;

    this->data_size_ = kDataSize;

    RequestParameters parameters;
    parameters.put("timeout", kTimeoutMs);
    parameters.put("batch_size", batch_size);

    this->batcher_.emplace(&parameters);
    this->batcher_->setName("test");
    this->batcher_->setBatchSize(batch_size);

    this->worker_.emplace("", &parameters);
    for (size_t i = 0; i < kBufferNum; i++) {
      BufferPtrs vec;
      vec.emplace_back(std::make_unique<VectorBuffer>(batch_size * kDataSize,
                                                      DataType::UINT8));
      this->worker_->putInputBuffer(std::move(vec));
    }
    for (size_t i = 0; i < kBufferNum; i++) {
      BufferPtrs vec;
      vec.emplace_back(std::make_unique<VectorBuffer>(batch_size * kDataSize,
                                                      DataType::UINT8));
      this->worker_->putOutputBuffer(std::move(vec));
    }

    this->batcher_->start(&this->worker_.value());

    data_.resize(kDataSize);
    for (auto i = 0; i < kDataSize; i++) {
      data_[i] = i;
    }

    this->request_ = InferenceRequest();
    this->request_.addInputTensor(static_cast<void*>(data_.data()), kDataShape,
                                  DataType::UINT8);
  }

  void TearDown() override {
    batcher_->enqueue(nullptr);
    batcher_->end();
  }

  void check_batch(int batches, int batch_size) {
    for (auto i = 0; i < batches; i++) {
      BatchPtr batch;
      ASSERT_EQ(
        batcher_->getOutputQueue()->wait_dequeue_timed(batch, kTimeoutUs),
        true);

      EXPECT_EQ(batch->input_buffers->size(), 1);
      EXPECT_EQ(batch->output_buffers->size(), 1);
      EXPECT_EQ(batch->requests->size(), batch_size);

      for (const auto& buffer : *(batch->input_buffers)) {
        EXPECT_EQ(buffer.size(), 1);
        auto& first_tensor = buffer[0];
        for (auto j = 0; j < batch_size; j++) {
          for (auto k = 0; k < data_size_; k++) {
            auto data = *(static_cast<uint8_t*>(first_tensor->data(k)));
            EXPECT_EQ(data, k);
          }
        }
      }
    }
  }

  int data_size_;
  std::vector<uint8_t> data_;
  InferenceRequest request_;
  std::optional<WorkerInfo> worker_;
  std::optional<SoftBatcher> batcher_;
};

TEST_P(UnitSoftBatcherFixture, BasicBatching) {
  auto [batch_size, num_requests] = GetParam();
  const auto kLeftover = static_cast<int>(num_requests % batch_size != 0);
  const auto kBatchCount = num_requests / batch_size + kLeftover;

  for (auto i = 0; i < num_requests; i++) {
    auto req = std::make_unique<CppNativeApi>(this->request_);
    batcher_->enqueue(std::move(req));
  }

  this->check_batch(kBatchCount - kLeftover, batch_size);
  if (kLeftover) {
    this->check_batch(kLeftover, num_requests % batch_size);
  }
}

// pairs of (batch_size, num_requests)
std::pair<int, int> configs[] = {{1, 1}, {1, 2}, {2, 1}, {2, 2}, {2, 3}};
INSTANTIATE_TEST_SUITE_P(Datatypes, UnitSoftBatcherFixture,
                         testing::ValuesIn(configs));

}  // namespace proteus
