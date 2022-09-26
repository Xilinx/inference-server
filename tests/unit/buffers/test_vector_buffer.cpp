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
#include "proteus/declarations.hpp"           // for BufferPtrs
#include "proteus/util/queue.hpp"             // for BufferPtrsQueue

namespace proteus {

constexpr auto kBufferSize = 10;
// constexpr auto kDataType = DataType::INT32;
// constexpr auto kDataSize = types::getSize(kDataType);
constexpr auto kTimeout = 1E6;  // 1E6 us = 1 s timeout

struct WriteData {
  template <typename T>
  size_t operator()(Buffer* buffer, int value, size_t offset) const {
    if constexpr (std::is_same_v<T, char>) {
      (void)buffer, (void)value, (void)offset;
      return offset;
    } else {
      return buffer->write(static_cast<T>(value), offset);
    }
  }
};

struct ReadData {
  template <typename T>
  void operator()(Buffer* buffer, size_t offset, int golden) const {
    if constexpr (std::is_same_v<T, char>) {
      (void)buffer, (void)offset, (void)golden;
      FAIL() << "Unhandled datatype: char ";
    } else {
      EXPECT_EQ(*static_cast<T*>(buffer->data(offset)), static_cast<T>(golden));
    }
  }
};

class UnitVectorBufferFixture : public testing::TestWithParam<DataType> {
 public:
  UnitVectorBufferFixture() : buffer_(kBufferSize, GetParam()){};

 protected:
  void SetUp() override {
    size_t offset = 0;
    for (auto i = 0; i < kBufferSize; i++) {
      offset = switchOverTypes(WriteData(), GetParam(), &buffer_, i, offset);
    }
  }

  // void TearDown() override {}

  VectorBuffer buffer_;
};

TEST_P(UnitVectorBufferFixture, TestFixtureState) {
  auto datatype = GetParam();
  for (auto i = 0; i < kBufferSize; i++) {
    switchOverTypes(ReadData(), datatype, &buffer_, i * datatype.size(), i);
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

// we exclude STRING as it doesn't have a defined size we can pre-allocate
DataType datatypes[] = {proteus::DataType::BOOL,   proteus::DataType::UINT8,
                        proteus::DataType::UINT16, proteus::DataType::UINT32,
                        proteus::DataType::UINT64, proteus::DataType::INT8,
                        proteus::DataType::INT16,  proteus::DataType::INT32,
                        proteus::DataType::INT64,  proteus::DataType::FP16,
                        proteus::DataType::FP32,   proteus::DataType::FP64};
INSTANTIATE_TEST_SUITE_P(Datatypes, UnitVectorBufferFixture,
                         testing::ValuesIn(datatypes));

}  // namespace proteus
