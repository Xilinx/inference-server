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

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>

#include "proteus/core/data_types.hpp"

namespace proteus {

using types::DataType;

size_t Buffer::write(void* data, size_t offset, size_t size) {
  std::memcpy(this->data(offset), data, size);
  return offset + size;
}

size_t Buffer::write(bool value, size_t offset) {
  constexpr auto bytes = types::getSize(DataType::BOOL);
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(uint8_t value, size_t offset) {
  constexpr auto bytes = types::getSize(DataType::UINT8);
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(uint16_t value, size_t offset) {
  constexpr auto bytes = types::getSize(DataType::UINT16);
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(uint32_t value, size_t offset) {
  constexpr auto bytes = types::getSize(DataType::UINT32);
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(uint64_t value, size_t offset) {
  constexpr auto bytes = types::getSize(DataType::UINT64);
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(int8_t value, size_t offset) {
  constexpr auto bytes = types::getSize(DataType::INT8);
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(int16_t value, size_t offset) {
  constexpr auto bytes = types::getSize(DataType::INT16);
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(int32_t value, size_t offset) {
  constexpr auto bytes = types::getSize(DataType::INT32);
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(int64_t value, size_t offset) {
  constexpr auto bytes = types::getSize(DataType::INT64);
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(float value, size_t offset) {
  constexpr auto bytes = types::getSize(DataType::FP32);
  std::memcpy(this->data(offset), &value, bytes);
  return offset + bytes;
}
size_t Buffer::write(double value, size_t offset) {
  constexpr auto bytes = types::getSize(DataType::FP64);
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
