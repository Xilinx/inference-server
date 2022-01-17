#include "proteus/batching/soft.hpp"

#include <chrono>
#include <future>
#include <memory>

#include "gtest/gtest.h"
#include "proteus/buffers/vector_buffer.hpp"
#include "proteus/core/worker_info.hpp"

namespace proteus {

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
      batcher_->enqueue(request_);
      count++;

      auto end_time = std::chrono::high_resolution_clock::now();
      duration = end_time - start_time;
    } while (duration < std::chrono::nanoseconds(std::chrono::seconds(1)));

    return count;
  }

  int dequeue() {
    const auto [batch_size, num_buffers, delay] = GetParam();
    bool valid_read;
    int count = 0;
    do {
      BatchPtr batch;
      valid_read =
        batcher_->getOutputQueue()->wait_dequeue_timed(batch, kTimeoutUs);
      std::this_thread::sleep_for(std::chrono::microseconds(delay));
      if (valid_read) {
        count++;
        this->worker_->putInputBuffer(std::move((*(batch->input_buffers))[0]));
        this->worker_->putOutputBuffer(
          std::move((*(batch->output_buffers))[0]));
      }
    } while (valid_read);
    return count;
  }

 protected:
  void SetUp() override {
    const auto [batch_size, num_buffers, delay] = GetParam();

    const auto kBufferNum = static_cast<size_t>(num_buffers);
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

  int data_size_;
  std::vector<uint8_t> data_;
  InferenceRequestInput request_;
  std::optional<WorkerInfo> worker_;
  std::optional<SoftBatcher> batcher_;
};

TEST_P(PerfSoftBatcherFixture, BasicBatching) {
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

  EXPECT_NEAR(enqueue_count / batch_size, dequeue_count, enqueue_count * 0.01);

  std::cerr << "Enqueue count: " << enqueue_count << std::endl;
  std::cerr << "Dequeue count: " << dequeue_count << std::endl;
  std::cerr << "Throughput (req/s): " << throughput << std::endl;
}

int batch_sizes[] = {1, 2, 4};

int buffer_nums[] = {1, 10, 100};

// delay after dequeue per request in microseconds
int work_delay[] = {0, 1, 10};

INSTANTIATE_TEST_SUITE_P(Datatypes, PerfSoftBatcherFixture,
                         testing::Combine(testing::ValuesIn(batch_sizes),
                                          testing::ValuesIn(buffer_nums),
                                          testing::ValuesIn(work_delay)));

}  // namespace proteus
