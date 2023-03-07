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

#include <benchmark/benchmark.h>

#include <array>     // for array
#include <chrono>    // for duration, operator-
#include <cstddef>   // for size_t
#include <cstdint>   // for uint8_t, uint64_t
#include <future>    // for async, future, launch
#include <iostream>  // for operator<<, basic_ost...
#include <memory>    // for allocator, make_unique
#include <optional>  // for optional
#include <ratio>     // for ratio
#include <string>    // for string
#include <thread>    // for sleep_for
#include <tuple>     // for tuple
#include <utility>   // for move
#include <vector>    // for vector

#include "amdinfer/batching/soft.hpp"          // for BatchPtr, SoftBatcher
#include "amdinfer/core/data_types.hpp"        // for DataType, DataType::U...
#include "amdinfer/core/memory_pool/pool.hpp"  // for MemoryPool
#include "amdinfer/core/parameters.hpp"        // for ParameterMap
#include "amdinfer/core/predict_api.hpp"       // for InferenceRequest, Req...
#include "amdinfer/core/predict_api_internal.hpp"  // for RequestContainer
#include "amdinfer/core/worker_info.hpp"           // for WorkerInfo
#include "amdinfer/declarations.hpp"               // for BufferPtrs
#include "amdinfer/observation/logging.hpp"        // for LogOptions, initLogger
#include "gtest/gtest.h"

namespace amdinfer {

constexpr auto kTimeoutMs = 1000;  // timeout in ms

class PerfSoftBatcherFixture : public ::benchmark::Fixture {
 public:
  void enqueue(int count) {
    for (auto i = 0; i < count; ++i) {
      auto req = std::make_unique<RequestContainer>();
      req->request = this->request_;
      batcher_->enqueue(std::move(req));
    }
  }

  void dequeue(int count, int delay) {
    for (auto i = 0; i < count; ++i) {
      BatchPtr batch;
      batcher_->getOutputQueue()->wait_dequeue(batch);
      std::this_thread::sleep_for(std::chrono::microseconds(delay));
    }
  }

  void SetUp(const ::benchmark::State& state) override {
    auto batch_size = static_cast<int>(state.range(0));

    const auto data_shape = {1UL, 2UL, 50UL};
    const auto data_size = 100;

    ParameterMap parameters;
    parameters.put("timeout", kTimeoutMs);
    parameters.put("batch_size", batch_size);

    LogOptions options;
    options.logger_name = "server";
    options.console_enable = true;
    options.file_enable = false;
    initLogger(options);

    this->batcher_.emplace(&pool_, &parameters);
    this->batcher_->setName("test");
    this->batcher_->setBatchSize(batch_size);

    this->worker_.emplace("", &parameters, &pool_);
    // for (size_t i = 0; i < buffer_num; i++) {
    //   BufferPtrs vec;
    //   vec.emplace_back(std::make_unique<VectorBuffer>(batch_size * data_size,
    //                                                   DataType::Uint8));
    //   this->worker_->putInputBuffer(std::move(vec));
    // }
    // for (size_t i = 0; i < buffer_num; i++) {
    //   BufferPtrs vec;
    //   vec.emplace_back(std::make_unique<VectorBuffer>(batch_size * data_size,
    //                                                   DataType::Uint8));
    //   this->worker_->putOutputBuffer(std::move(vec));
    // }

    this->batcher_->start({MemoryAllocators::Cpu});

    data_.resize(data_size);
    for (auto i = 0; i < data_size; i++) {
      data_[i] = static_cast<uint8_t>(i);
    }

    request_ = std::make_shared<InferenceRequest>();
    request_->addInputTensor(static_cast<void*>(data_.data()), data_shape,
                             DataType::Uint8);
  }

  void TearDown([[maybe_unused]] const ::benchmark::State& state) override {
    batcher_->enqueue(nullptr);
    batcher_->end();
  }

 private:
  std::vector<uint8_t> data_;
  InferenceRequestPtr request_;
  std::optional<WorkerInfo> worker_;
  std::optional<SoftBatcher> batcher_;
  MemoryPool pool_;
};

BENCHMARK_DEFINE_F(PerfSoftBatcherFixture, BasicBatching)
(benchmark::State& st) {  // NOLINT

  const auto enqueue_count = 1000;

  for ([[maybe_unused]] auto _ : st) {
    auto batch_size = static_cast<int>(st.range(0));
    const auto dequeue_count = static_cast<int>(
      std::ceil(enqueue_count / static_cast<double>(batch_size)));
    auto enqueue =
      std::async(std::launch::async, &PerfSoftBatcherFixture::enqueue, this,
                 enqueue_count);
    auto dequeue =
      std::async(std::launch::async, &PerfSoftBatcherFixture::dequeue, this,
                 dequeue_count, st.range(2));

    enqueue.get();
    dequeue.get();
  }
}

const std::initializer_list<int64_t> kBatchSizes{1, 2, 4};

// delay after dequeue per request in microseconds
const std::initializer_list<int64_t> kWorkDelay{0, 1, 10};

// NOLINTNEXTLINE(cert-err58-cpp, cppcoreguidelines-owning-memory)
BENCHMARK_REGISTER_F(PerfSoftBatcherFixture, BasicBatching)
  ->ArgsProduct({kBatchSizes, kWorkDelay})
  ->Unit(benchmark::kMillisecond);

// NOLINTNEXTLINE
BENCHMARK_MAIN();

}  // namespace amdinfer
