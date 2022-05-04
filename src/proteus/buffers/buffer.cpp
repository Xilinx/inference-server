// Copyright 2021 Xilinx Inc.
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

/**
 * @file
 * @brief Implements the Buffer class
 */

#include "proteus/buffers/buffer.hpp"

#include <algorithm>  // for copy
#include <cstring>    // for memcpy, size_t
#include <string>     // for string

#include "proteus/core/data_types.hpp"  // for getSize, DataType, DataType::...

namespace proteus {

size_t Buffer::write(void* data, size_t offset, size_t size) {
  std::memcpy(this->data(offset), data, size);
  return offset + size;
}

size_t Buffer::write(bool value, size_t offset) {
  constexpr auto bytes = DataType("BOOL").size();
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(uint8_t value, size_t offset) {
  constexpr auto bytes = DataType("UINT8").size();
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(uint16_t value, size_t offset) {
  constexpr auto bytes = DataType("UINT16").size();
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(uint32_t value, size_t offset) {
  constexpr auto bytes = DataType("UINT32").size();
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(uint64_t value, size_t offset) {
  constexpr auto bytes = DataType("UINT64").size();
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(int8_t value, size_t offset) {
  constexpr auto bytes = DataType("INT8").size();
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(int16_t value, size_t offset) {
  constexpr auto bytes = DataType("INT16").size();
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(int32_t value, size_t offset) {
  constexpr auto bytes = DataType("INT32").size();
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(int64_t value, size_t offset) {
  constexpr auto bytes = DataType("INT64").size();
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(float value, size_t offset) {
  constexpr auto bytes = DataType("FP32").size();
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(double value, size_t offset) {
  constexpr auto bytes = DataType("FP64").size();
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
// not quite sure what the best way to do this is. The commented out code is
// other attempts. This works but may be non-optimal.
size_t Buffer::write(std::string value, size_t offset) {
  // constexpr auto bytes = types::getSize(DataType::STRING);
  // std::memcpy(this->data(offset), &value, bytes);

  char null_term = '\0';
  std::copy(value.begin(), value.end(),
            reinterpret_cast<char*>(this->data(offset)));
  // strcpy(reinterpret_cast<char*>(static_cast<std::byte*>(this->data()) +
  // this->write_counter_),
  // value.c_str());
  std::memcpy(static_cast<std::byte*>(this->data(offset)) + value.length(),
              &null_term, 1);

  // this->write_counter_ += value.length() + 1;
  return offset + value.length() + 1;
}

}  // namespace proteus
