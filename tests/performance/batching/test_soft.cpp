// Copyright 2021 Xilinx, Inc.
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

#include <chrono>    // for duration, operator-
#include <cstddef>   // for size_t
#include <cstdint>   // for uint8_t, uint64_t
#include <future>    // for async, future, launch
#include <iostream>  // for operator<<, basic_ost...
#include <memory>    // for allocator, make_unique
#include <optional>  // for optional
#include <ratio>     // for ratio
#include <thread>    // for sleep_for
#include <tuple>     // for tuple
#include <utility>   // for move
#include <vector>    // for vector

#include "amdinfer/batching/soft.hpp"            // for BatchPtr, SoftBatcher
#include "amdinfer/buffers/vector_buffer.hpp"    // for VectorBuffer
#include "amdinfer/clients/native_internal.hpp"  // for CppNativeApi
#include "amdinfer/core/data_types.hpp"          // for DataType, DataType::U...
#include "amdinfer/core/predict_api.hpp"         // for InferenceRequest, Req...
#include "amdinfer/core/worker_info.hpp"         // for WorkerInfo
#include "amdinfer/declarations.hpp"             // for BufferPtrs
#include "gtest/gtest.h"                         // for UnitTest, EXPECT_NEAR

namespace amdinfer {

constexpr auto kTimeoutMs = 1000;  // timeout in ms
// timeout in us with safety factor
constexpr auto kTimeoutUs = kTimeoutMs * 1000 * 5;

class PerfSoftBatcherFixture
  : public testing::TestWithParam<std::tuple<int, int, int>> {
 public:
  int enqueue() {
    auto start_time = std::chrono::high_resolution_clock::now();
    std::chrono::nanoseconds duration = std::chrono::nanoseconds(0);

    int count = 0;

    do {
      auto req = std::make_unique<CppNativeApi>(this->request_);
      batcher_->enqueue(std::move(req));
      count++;

      auto end_time = std::chrono::high_resolution_clock::now();
      duration = end_time - start_time;
    } while (duration < std::chrono::nanoseconds(std::chrono::seconds(1)));

    return count;
  }

  int dequeue() {
    const auto [batch_size, num_buffers, delay] = GetParam();
    bool valid_read = false;
    int count = 0;
    do {
      BatchPtr batch;
      valid_read =
        batcher_->getOutputQueue()->wait_dequeue_timed(batch, kTimeoutUs);
      std::this_thread::sleep_for(std::chrono::microseconds(delay));
      if (valid_read) {
        count++;
      }
    } while (valid_read);
    return count;
  }

 protected:
  void SetUp() override {
    const auto [batch_size, num_buffers, delay] = GetParam();

    const auto buffer_num = static_cast<size_t>(num_buffers);
    const auto data_shape = {1UL, 2UL, 50UL};
    const auto data_size = 100;

    this->data_size_ = data_size;

    RequestParameters parameters;
    parameters.put("timeout", kTimeoutMs);
    parameters.put("batch_size", batch_size);

    LogOptions options;
    options.logger_name = "server";
    options.console_enable = true;
    options.file_enable = false;
    initLogger(options);

    this->batcher_.emplace(&parameters);
    this->batcher_->setName("test");
    this->batcher_->setBatchSize(batch_size);

    this->worker_.emplace("", &parameters);
    for (size_t i = 0; i < buffer_num; i++) {
      BufferPtrs vec;
      vec.emplace_back(std::make_unique<VectorBuffer>(batch_size * data_size,
                                                      DataType::Uint8));
      this->worker_->putInputBuffer(std::move(vec));
    }
    for (size_t i = 0; i < buffer_num; i++) {
      BufferPtrs vec;
      vec.emplace_back(std::make_unique<VectorBuffer>(batch_size * data_size,
                                                      DataType::Uint8));
      this->worker_->putOutputBuffer(std::move(vec));
    }

    this->batcher_->start(&this->worker_.value());

    data_.resize(data_size);
    for (auto i = 0; i < data_size; i++) {
      data_[i] = i;
    }

    this->request_ = InferenceRequest();
    this->request_.addInputTensor(static_cast<void*>(data_.data()), data_shape,
                                  DataType::Uint8);
  }

  void TearDown() override {
    batcher_->enqueue(nullptr);
    batcher_->end();
  }

  int data_size_ = 0;
  std::vector<uint8_t> data_;
  InferenceRequest request_;
  std::optional<WorkerInfo> worker_;
  std::optional<SoftBatcher> batcher_;
};

// @pytest.mark.perf(group="batcher")
TEST_P(PerfSoftBatcherFixture, BasicBatching) {  // NOLINT
  const auto [batch_size, num_buffers, delay] = GetParam();
  auto start_time = std::chrono::high_resolution_clock::now();

  auto enqueue =
    std::async(std::launch::async, &PerfSoftBatcherFixture::enqueue, this);
  auto dequeue =
    std::async(std::launch::async, &PerfSoftBatcherFixture::dequeue, this);

  auto enqueue_count = enqueue.get();
  auto dequeue_count = dequeue.get();

  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end_time - start_time;
  auto throughput = static_cast<double>(enqueue_count) / duration.count();

  EXPECT_NEAR(enqueue_count / float(batch_size), dequeue_count,
              enqueue_count * 0.01);

  std::cerr << "Enqueue count: " << enqueue_count << std::endl;
  std::cerr << "Dequeue count: " << dequeue_count << std::endl;
  std::cerr << "Throughput (req/s): " << throughput << std::endl;
}

const std::array batch_sizes{1, 2, 4};

const std::array buffer_nums{1, 10, 100};

// delay after dequeue per request in microseconds
const std::array work_delay{0, 1, 10};

// NOLINTNEXTLINE(cert-err58-cpp)
INSTANTIATE_TEST_SUITE_P(Datatypes, PerfSoftBatcherFixture,
                         testing::Combine(testing::ValuesIn(batch_sizes),
                                          testing::ValuesIn(buffer_nums),
                                          testing::ValuesIn(work_delay)));

}  // namespace amdinfer
