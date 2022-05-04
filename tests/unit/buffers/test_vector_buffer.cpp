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

#include <cstddef>  // for size_t
#include <cstdint>  // for uint16_t, int16_t, int32_t
#include <memory>   // for allocator

#include "gtest/gtest.h"                      // for EXPECT_EQ, FAIL, UnitTest
#include "proteus/buffers/vector_buffer.hpp"  // for VectorBuffer
#include "proteus/core/data_types.hpp"        // for DataType, getSize, oper...
#include "proteus/helpers/declarations.hpp"   // for BufferPtrs
#include "proteus/helpers/queue.hpp"          // for BufferPtrsQueue

namespace proteus {

constexpr auto kBufferSize = 10;
// constexpr auto kDataType = DataType::INT32;
// constexpr auto kDataSize = types::getSize(kDataType);
constexpr auto kTimeout = 1E6;  // 1E6 us = 1 s timeout

class UnitVectorBufferFixture : public testing::TestWithParam<DataType> {
 public:
  UnitVectorBufferFixture() : buffer_(kBufferSize, GetParam()){};

 protected:
  void SetUp() override {
    size_t offset = 0;
    for (auto i = 0; i < kBufferSize; i++) {
      switch (GetParam()) {
        case DataType::BOOL:
          offset = buffer_.write(false, offset);
          break;
        case DataType::UINT8:
          offset = buffer_.write(static_cast<uint8_t>(0), offset);
          break;
        case DataType::UINT16:
          offset = buffer_.write(static_cast<uint16_t>(0), offset);
          break;
        case DataType::UINT32:
          offset = buffer_.write(static_cast<uint32_t>(0), offset);
          break;
        case DataType::UINT64:
          offset = buffer_.write(static_cast<uint64_t>(0), offset);
          break;
        case DataType::INT8:
          offset = buffer_.write(static_cast<int8_t>(0), offset);
          break;
        case DataType::INT16:
          offset = buffer_.write(static_cast<int16_t>(0), offset);
          break;
        case DataType::INT32:
          offset = buffer_.write(static_cast<int32_t>(0), offset);
          break;
        case DataType::INT64:
          offset = buffer_.write(static_cast<int64_t>(0), offset);
          break;
        case DataType::FP32:
          offset = buffer_.write(static_cast<float>(0), offset);
          break;
        case DataType::FP64:
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
  auto datatype = GetParam();
  for (auto i = 0; i < kBufferSize; i++) {
    switch (datatype) {
      case DataType::BOOL: {
        auto value = static_cast<bool*>(buffer_.data(i * datatype.size()));
        EXPECT_EQ(*value, false);
        break;
      }
      case DataType::UINT8: {
        auto value = static_cast<uint8_t*>(buffer_.data(i * datatype.size()));
        EXPECT_EQ(*value, static_cast<uint8_t>(0));
        break;
      }
      case DataType::UINT16: {
        auto value = static_cast<uint16_t*>(buffer_.data(i * datatype.size()));
        EXPECT_EQ(*value, static_cast<uint16_t>(0));
        break;
      }
      case DataType::UINT32: {
        auto value = static_cast<uint32_t*>(buffer_.data(i * datatype.size()));
        EXPECT_EQ(*value, static_cast<uint32_t>(0));
        break;
      }
      case DataType::UINT64: {
        auto value = static_cast<uint64_t*>(buffer_.data(i * datatype.size()));
        EXPECT_EQ(*value, static_cast<uint64_t>(0));
        break;
      }
      case DataType::INT8: {
        auto value = static_cast<int8_t*>(buffer_.data(i * datatype.size()));
        EXPECT_EQ(*value, static_cast<int8_t>(0));
        break;
      }
      case DataType::INT16: {
        auto value = static_cast<int16_t*>(buffer_.data(i * datatype.size()));
        EXPECT_EQ(*value, static_cast<int16_t>(0));
        break;
      }
      case DataType::INT32: {
        auto value = static_cast<int32_t*>(buffer_.data(i * datatype.size()));
        EXPECT_EQ(*value, static_cast<int32_t>(0));
        break;
      }
      case DataType::INT64: {
        auto value = static_cast<uint16_t*>(buffer_.data(i * datatype.size()));
        EXPECT_EQ(*value, static_cast<uint16_t>(0));
        break;
      }
      case DataType::FP32: {
        auto value = static_cast<float*>(buffer_.data(i * datatype.size()));
        EXPECT_EQ(*value, static_cast<float>(0));
        break;
      }
      case DataType::FP64: {
        auto value = static_cast<double*>(buffer_.data(i * datatype.size()));
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
DataType datatypes[] = {proteus::DataType::BOOL,   proteus::DataType::UINT8,
                        proteus::DataType::UINT16, proteus::DataType::UINT32,
                        proteus::DataType::UINT64, proteus::DataType::INT8,
                        proteus::DataType::INT16,  proteus::DataType::INT32,
                        proteus::DataType::INT64,  proteus::DataType::FP32,
                        proteus::DataType::FP64};
INSTANTIATE_TEST_SUITE_P(Datatypes, UnitVectorBufferFixture,
                         testing::ValuesIn(datatypes));

}  // namespace proteus
