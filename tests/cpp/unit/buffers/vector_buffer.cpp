#include "proteus/buffers/vector_buffer.hpp"

#include "gtest/gtest.h"
#include "proteus/core/data_types.hpp"

namespace proteus {

constexpr auto kBufferSize = 10;
// constexpr auto kDataType = types::DataType::INT32;
// constexpr auto kDataSize = types::getSize(kDataType);
constexpr auto kTimeout = 1E6;  // 1E6 us = 1 s timeout

class UnitVectorBufferFixture : public testing::TestWithParam<types::DataType> {
 public:
  UnitVectorBufferFixture() : buffer_(kBufferSize, GetParam()){};

 protected:
  void SetUp() override {
    size_t offset = 0;
    for (auto i = 0; i < kBufferSize; i++) {
      switch (GetParam()) {
        case types::DataType::BOOL:
          offset = buffer_.write(false, offset);
          break;
        case types::DataType::UINT8:
          offset = buffer_.write(static_cast<uint8_t>(0), offset);
          break;
        case types::DataType::UINT16:
          offset = buffer_.write(static_cast<uint16_t>(0), offset);
          break;
        case types::DataType::UINT32:
          offset = buffer_.write(static_cast<uint32_t>(0), offset);
          break;
        case types::DataType::UINT64:
          offset = buffer_.write(static_cast<uint64_t>(0), offset);
          break;
        case types::DataType::INT8:
          offset = buffer_.write(static_cast<int8_t>(0), offset);
          break;
        case types::DataType::INT16:
          offset = buffer_.write(static_cast<int16_t>(0), offset);
          break;
        case types::DataType::INT32:
          offset = buffer_.write(static_cast<int32_t>(0), offset);
          break;
        case types::DataType::INT64:
          offset = buffer_.write(static_cast<int64_t>(0), offset);
          break;
        case types::DataType::FP32:
          offset = buffer_.write(static_cast<float>(0), offset);
          break;
        case types::DataType::FP64:
          offset = buffer_.write(static_cast<double>(0), offset);
          break;
        default:
          FAIL() << "Unhandled datatype: " << GetParam();
          break;
      }
    }
  }

  // void TearDown() override {}

  VectorBuffer buffer_;
};

TEST_P(UnitVectorBufferFixture, TestFixtureState) {
  for (auto i = 0; i < kBufferSize; i++) {
    switch (GetParam()) {
      case types::DataType::BOOL: {
        auto value =
          static_cast<bool*>(buffer_.data(i * types::getSize(GetParam())));
        EXPECT_EQ(*value, false);
        break;
      }
      case types::DataType::UINT8: {
        auto value =
          static_cast<uint8_t*>(buffer_.data(i * types::getSize(GetParam())));
        EXPECT_EQ(*value, static_cast<uint8_t>(0));
        break;
      }
      case types::DataType::UINT16: {
        auto value =
          static_cast<uint16_t*>(buffer_.data(i * types::getSize(GetParam())));
        EXPECT_EQ(*value, static_cast<uint16_t>(0));
        break;
      }
      case types::DataType::UINT32: {
        auto value =
          static_cast<uint32_t*>(buffer_.data(i * types::getSize(GetParam())));
        EXPECT_EQ(*value, static_cast<uint32_t>(0));
        break;
      }
      case types::DataType::UINT64: {
        auto value =
          static_cast<uint64_t*>(buffer_.data(i * types::getSize(GetParam())));
        EXPECT_EQ(*value, static_cast<uint64_t>(0));
        break;
      }
      case types::DataType::INT8: {
        auto value =
          static_cast<int8_t*>(buffer_.data(i * types::getSize(GetParam())));
        EXPECT_EQ(*value, static_cast<int8_t>(0));
        break;
      }
      case types::DataType::INT16: {
        auto value =
          static_cast<int16_t*>(buffer_.data(i * types::getSize(GetParam())));
        EXPECT_EQ(*value, static_cast<int16_t>(0));
        break;
      }
      case types::DataType::INT32: {
        auto value =
          static_cast<int32_t*>(buffer_.data(i * types::getSize(GetParam())));
        EXPECT_EQ(*value, static_cast<int32_t>(0));
        break;
      }
      case types::DataType::INT64: {
        auto value =
          static_cast<uint16_t*>(buffer_.data(i * types::getSize(GetParam())));
        EXPECT_EQ(*value, static_cast<uint16_t>(0));
        break;
      }
      case types::DataType::FP32: {
        auto value =
          static_cast<float*>(buffer_.data(i * types::getSize(GetParam())));
        EXPECT_EQ(*value, static_cast<float>(0));
        break;
      }
      case types::DataType::FP64: {
        auto value =
          static_cast<double*>(buffer_.data(i * types::getSize(GetParam())));
        EXPECT_EQ(*value, static_cast<double>(0));
        break;
      }
      default:
        FAIL() << "Unhandled datatype: " << GetParam();
        break;
    }
  }
}

TEST_P(UnitVectorBufferFixture, TestAllocate) {
  BufferPtrsQueue buffer_queue;
  const size_t kBufferNum = 10;
  const size_t kBatchSize = 4;

  VectorBuffer::allocate(&buffer_queue, kBufferNum, 1 * kBatchSize, GetParam());

  for (auto i = 0U; i < kBufferNum; i++) {
    BufferPtrs buffers;
    if (!buffer_queue.wait_dequeue_timed(buffers, kTimeout)) {
      FAIL() << "Dequeuing buffers timed out";
    }
    EXPECT_EQ(buffers.size(), 1);
  }
}

// we exclude FP16 and STRING. FP16 isn't supported and STRING doesn't have
// a defined size we can pre-allocate
types::DataType datatypes[] = {
  proteus::types::DataType::BOOL,   proteus::types::DataType::UINT8,
  proteus::types::DataType::UINT16, proteus::types::DataType::UINT32,
  proteus::types::DataType::UINT64, proteus::types::DataType::INT8,
  proteus::types::DataType::INT16,  proteus::types::DataType::INT32,
  proteus::types::DataType::INT64,  proteus::types::DataType::FP32,
  proteus::types::DataType::FP64};
INSTANTIATE_TEST_SUITE_P(Datatypes, UnitVectorBufferFixture,
                         testing::ValuesIn(datatypes));

}  // namespace proteus
