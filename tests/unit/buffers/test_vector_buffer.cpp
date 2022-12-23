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

#include <cstddef>  // for size_t
#include <cstdint>  // for uint16_t, int16_t, int32_t
#include <memory>   // for allocator

#include "amdinfer/buffers/vector_buffer.hpp"  // for VectorBuffer
#include "amdinfer/core/data_types.hpp"        // for DataType, getSize, oper...
#include "amdinfer/declarations.hpp"           // for BufferPtrs
#include "amdinfer/util/queue.hpp"             // for BufferPtrsQueue
#include "gtest/gtest.h"                       // for EXPECT_EQ, FAIL, UnitTest

namespace amdinfer {

constexpr auto kBufferSize = 10;
// constexpr auto kDataType = DataType::Int32;
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

TEST_P(UnitVectorBufferFixture, TestFixtureState) {  // NOLINT
  auto datatype = GetParam();
  for (auto i = 0; i < kBufferSize; i++) {
    switchOverTypes(ReadData(), datatype, &buffer_, i * datatype.size(), i);
  }
}

TEST_P(UnitVectorBufferFixture, TestAllocate) {  // NOLINT
  BufferPtrsQueue buffer_queue;
  const size_t buffer_num = 10;
  const size_t batch_size = 4;

  VectorBuffer::allocate(&buffer_queue, buffer_num, 1 * batch_size, GetParam());

  for (auto i = 0U; i < buffer_num; i++) {
    BufferPtrs buffers;
    if (!buffer_queue.wait_dequeue_timed(buffers, kTimeout)) {
      FAIL() << "Dequeuing buffers timed out";
    }
    EXPECT_EQ(buffers.size(), 1);
  }
}

// we exclude STRING as it doesn't have a defined size we can pre-allocate
// NOLINTNEXTLINE(cert-err58-cpp)
const std::array<DataType, 12> kDataTypes{
  amdinfer::DataType::Bool,   amdinfer::DataType::Uint8,
  amdinfer::DataType::Uint16, amdinfer::DataType::Uint32,
  amdinfer::DataType::Uint64, amdinfer::DataType::Int8,
  amdinfer::DataType::Int16,  amdinfer::DataType::Int32,
  amdinfer::DataType::Int64,  amdinfer::DataType::Fp16,
  amdinfer::DataType::Fp32,   amdinfer::DataType::Fp64};

// NOLINTNEXTLINE(cert-err58-cpp)
INSTANTIATE_TEST_SUITE_P(DataTypes, UnitVectorBufferFixture,
                         testing::ValuesIn(kDataTypes));

}  // namespace amdinfer
