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

#include <array>             // for array
#include <cstddef>           // for size_t
#include <cstdint>           // for uint8_t, uint64_t
#include <initializer_list>  // for initializer_list
#include <memory>            // for allocator, make_unique
#include <optional>          // for optional
#include <ostream>           // for operator<<, basic_ost...
#include <utility>           // for move
#include <vector>            // for vector

#include "amdinfer/batching/soft.hpp"          // for BatchPtr, SoftBatcher
#include "amdinfer/build_options.hpp"          // for AMDINFER_ENABLE_LOGGING
#include "amdinfer/core/data_types.hpp"        // for DataType, DataType::U...
#include "amdinfer/core/memory_pool/pool.hpp"  // for MemoryPool
#include "amdinfer/core/parameters.hpp"        // for ParameterMap
#include "amdinfer/core/predict_api_internal.hpp"  // for InferenceRequest, Req...
#include "amdinfer/core/worker_info.hpp"           // for WorkerInfo
#include "amdinfer/declarations.hpp"               // for BufferPtrs
#include "amdinfer/observation/logging.hpp"        // for initLogger, LogLevel
#include "gtest/gtest.h"                           // for ParamIteratorInterface

namespace amdinfer {

// timeout in ms for the batcher
constexpr auto kTimeoutMs = 1000;
// timeout in us with a safety factor of 10 to read from the batcher
constexpr auto kTimeoutUs = kTimeoutMs * 1000 * 10;

struct BatchConfig {
  int batch_size;
  std::initializer_list<int> requests;
  std::initializer_list<int> golden;

  friend std::ostream& operator<<(std::ostream& os, const BatchConfig& self) {
    os << "Batch Size: " << self.batch_size << ", ";
    os << "Requests: {";
    for (const auto& i : self.requests) {
      os << i << ",";
    }
    os << "}, Golden: {";
    for (const auto& i : self.golden) {
      os << i << ",";
    }
    os << "}";
    return os;
  }
};

class UnitSoftBatcherFixture : public testing::TestWithParam<BatchConfig> {
 protected:
  void SetUp() override {
#ifdef AMDINFER_ENABLE_LOGGING
    LogOptions options{
      "server",         // logger_name
      "",               // log directory
      false,            // enable file logging
      LogLevel::Debug,  // file log level
      true,             // enable console logging
      LogLevel::Warn    // console log level
    };
    initLogger(options);
#endif

    const auto& batch_config = GetParam();
    int batch_size = batch_config.batch_size;

    const auto data_shape = {1UL, 2UL, 50UL};
    // the product of the data shape < 255 to fit into uint8
    const uint8_t data_size = 100;

    this->data_size_ = data_size;
    this->data_shape_ = data_shape;

    ParameterMap parameters;
    parameters.put("timeout", kTimeoutMs);
    parameters.put("batch_size", batch_size);

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
    for (uint8_t i = 0; i < data_size; i++) {
      data_[i] = i;
    }
  }

  void TearDown() override {
    batcher_->enqueue(nullptr);
    batcher_->end();
  }

  void compareData(Buffer* tensor, int offset) const {
    for (auto k = 0; k < data_size_; k++) {
      auto data = *(static_cast<uint8_t*>(tensor->data(k + offset)));
      EXPECT_EQ(data, k);
    }
  }

  void checkBatch() {
    const auto& batch_config = GetParam();
    auto requests = std::vector(batch_config.requests);
    auto golden = batch_config.golden;

    int tensor_index = 0;
    for (const auto& i : golden) {
      BatchPtr batch;
      ASSERT_EQ(
        batcher_->getOutputQueue()->wait_dequeue_timed(batch, kTimeoutUs),
        true);

      EXPECT_EQ(batch->size(), i);

      auto num_tensors = 0;
      for (auto j = tensor_index; j < tensor_index + i; j++) {
        num_tensors += requests[j];
      }
      tensor_index += i;

      const auto& buffers = batch->getInputBuffers();
      EXPECT_EQ(buffers.size(), 1);
      for (const auto& buffer : buffers) {
        for (auto j = 0; j < num_tensors; j++) {
          compareData(buffer.get(), j * data_size_);
        }
      }
    }
  }

  void enqueue(std::unique_ptr<RequestContainer> req) {
    batcher_->enqueue(std::move(req));
  }

  InferenceRequestPtr createRequest() {
    auto request = std::make_shared<InferenceRequest>();
    request->addInputTensor(static_cast<void*>(data_.data()), data_shape_,
                            DataType::Uint8);
    return request;
  }

 private:
  int data_size_ = 0;
  std::vector<uint8_t> data_;
  std::initializer_list<uint64_t> data_shape_;
  InferenceRequestPtr request_;
  std::optional<WorkerInfo> worker_;
  std::optional<SoftBatcher> batcher_;
  MemoryPool pool_;
};

TEST_P(UnitSoftBatcherFixture, BasicBatching) {  // NOLINT
  const auto& batch_config = GetParam();
  auto requests = batch_config.requests;

  for (const auto& i : requests) {
    for (auto j = 0; j < i; ++j) {
      auto req = std::make_unique<RequestContainer>();
      req->request = createRequest();
      this->enqueue(std::move(req));
    }
  }

  this->checkBatch();
}

// batch size, requests, golden # tensors per response,
/**
 * The BatchConfig defines the configuration for the batcher. It has the
 * following interpretation:
 *
 * {a, {b0, b1...bn}, {c0, c1... cm}}
 *
 * a: the configured batch size for the batcher
 * b: this array defines the number of requests with request x having bx tensors
 * c: this array defines that batch x will have cx requests with all
 *    corresponding tensors associated with them from b
 *
 */
const std::array kConfigs{
  BatchConfig{1, {1}, {1}},          BatchConfig{1, {1, 1}, {1, 1}},
  BatchConfig{2, {1}, {1}},          BatchConfig{2, {1, 1}, {2}},
  BatchConfig{2, {1, 1, 1}, {2, 1}}, BatchConfig{4, {1, 1, 1, 1, 1}, {4, 1}},
  BatchConfig{4, {1, 1, 1, 1}, {4}},
};

// NOLINTNEXTLINE(cert-err58-cpp)
INSTANTIATE_TEST_SUITE_P(Datatypes, UnitSoftBatcherFixture,
                         testing::ValuesIn(kConfigs));

}  // namespace amdinfer
