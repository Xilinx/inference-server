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
 * @brief Defines how inference requests made to Proteus should store their
 * tensor data: using subclasses of the Buffer class
 */

#ifndef GUARD_PROTEUS_BUFFERS_BUFFER
#define GUARD_PROTEUS_BUFFERS_BUFFER

#include <cstddef>  // for size_t
#include <cstdint>  // for int16_t, int32_t, int64_t, int8_t, uint16_t, uint...
#include <cstring>  // for memcpy, size_t
#include <string>   // for string

namespace amdinfer {

/**
 * @brief The base buffer class used in Proteus. Buffer implementations should
 * extend this class and override the methods.
 */
class Buffer {
 public:
  /// Destroy the Proteus Buffer object
  virtual ~Buffer() = default;

  /**
   * @brief Get a pointer to the underlying data of the buffer
   *
   * @param offset For non-contiguous buffers, an offset may be needed to choose
   * which pointer to return. Buffer implementations may ignore this value if
   * unneeded.
   *
   * @return void*
   */
  virtual void* data(size_t offset = 0) = 0;

  /**
   * @brief Reset the buffer. This should be called prior to returning the
   * buffer to its buffer pool.
   */
  virtual void reset() = 0;

  /**
   * @brief Write arbitrary data from an address into this buffer.
   *
   * @param data pointer to data
   * @param offset offset to start writing the data
   * @param size size of the data to write in bytes
   * @return size_t number of bytes actually written
   */
  virtual size_t write(void* data, size_t offset, size_t size);

  /**
   * @brief Write a bool value to the buffer
   *
   * @param value value to write
   */
  template <typename T>
  size_t write(T value, size_t offset) {
    if constexpr (std::is_same_v<std::string, T>) {
      // not quite sure what the best way to do this is. The commented out code
      // is other attempts. This works but may be non-optimal.
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
    } else {
      std::memcpy(this->data(offset), &value, sizeof(T));
      return offset + sizeof(T);
    }
  }
};

}  // namespace amdinfer
#endif  // GUARD_PROTEUS_BUFFERS_BUFFER
