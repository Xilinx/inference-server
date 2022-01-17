#include "gtest/gtest.h"
#include "proteus/batching/soft.hpp"
#include "proteus/buffers/vector_buffer.hpp"
#include "proteus/core/worker_info.hpp"

namespace proteus {

constexpr auto kTimeoutMs = 1000;  // timeout in ms
constexpr auto kTimeoutUs =
  kTimeoutMs * 1000 * 1.5;  // timeout in us with buffer

class UnitSoftBatcherFixture
  : public testing::TestWithParam<std::pair<int, int>> {
 protected:
  void SetUp() override {
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
                                                      types::DataType::UINT8));
      this->worker_->putInputBuffer(std::move(vec));
    }
    for (size_t i = 0; i < kBufferNum; i++) {
      BufferPtrs vec;
      vec.emplace_back(std::make_unique<VectorBuffer>(batch_size * kDataSize,
                                                      types::DataType::UINT8));
      this->worker_->putOutputBuffer(std::move(vec));
    }

    this->batcher_->start(&this->worker_.value());

    data_.resize(kDataSize);
    for (auto i = 0; i < kDataSize; i++) {
      data_[i] = i;
    }

    this->request_ = InferenceRequestInput{static_cast<void*>(data_.data()),
                                           kDataShape, types::DataType::UINT8};
  }

  void TearDown() override { batcher_->end(); }

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
  InferenceRequestInput request_;
  std::optional<WorkerInfo> worker_;
  std::optional<SoftBatcher> batcher_;
};

TEST_P(UnitSoftBatcherFixture, BasicBatching) {
  auto [batch_size, num_requests] = GetParam();
  const auto kLeftover = static_cast<int>(num_requests % batch_size != 0);
  const auto kBatchCount = num_requests / batch_size + kLeftover;

  for (auto i = 0; i < num_requests; i++) {
    batcher_->enqueue(request_);
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
