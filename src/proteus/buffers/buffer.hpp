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
#include <cstdint>  // for int16_t, int32_t, int64_t
#include <string>

#include "proteus/build_options.hpp"        // for PROTEUS_ENABLE_LOGGING
#include "proteus/observation/logging.hpp"  // for getLogger, LoggerPtr

namespace proteus {

enum class Status;

/**
 * @brief The base buffer class used in Proteus. Buffer implementations should
 * extend this class and override the methods.
 */
class Buffer {
 public:
  /// Construct a new Proteus Buffer object
  Buffer() {}
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
  virtual size_t write(bool value, size_t offset);

  /**
   * @brief Write a uint8_t value to the buffer
   *
   * @param value value to write
   */
  virtual size_t write(uint8_t value, size_t offset);

  /**
   * @brief Write a uint16_t value to the buffer
   *
   * @param value value to write
   */
  virtual size_t write(uint16_t value, size_t offset);

  /**
   * @brief Write a uint32_t value to the buffer
   *
   * @param value value to write
   */
  virtual size_t write(uint32_t value, size_t offset);

  /**
   * @brief Write a uint64_t value to the buffer
   *
   * @param value value to write
   */
  virtual size_t write(uint64_t value, size_t offset);

  /**
   * @brief Write a int8_t value to the buffer
   *
   * @param value value to write
   */
  virtual size_t write(int8_t value, size_t offset);

  /**
   * @brief Write a int16_t value to the buffer
   *
   * @param value value to write
   */
  virtual size_t write(int16_t value, size_t offset);

  /**
   * @brief Write a int32_t value to the buffer
   *
   * @param value value to write
   */
  virtual size_t write(int32_t value, size_t offset);

  /**
   * @brief Write a int64_t value to the buffer
   *
   * @param value value to write
   */
  virtual size_t write(int64_t value, size_t offset);

  /**
   * @brief Write a float value to the buffer
   *
   * @param value value to write
   */
  virtual size_t write(float value, size_t offset);

  /**
   * @brief Write a double value to the buffer
   *
   * @param value value to write
   */
  virtual size_t write(double value, size_t offset);

  /**
   * @brief Write a string value to the buffer
   *
   * @param value value to write
   */
  virtual size_t write(std::string value, size_t offset);

  // /**
  //  * @brief Read a bool value from the buffer
  //  *
  //  * @param index address to read from
  //  * @param value holds the read value if read is successful
  //  * @return Status::kOK if successful
  //  */
  // virtual Status read(int index, bool& value) = 0;

  // /**
  //  * @brief Read a uint8_t value from the buffer
  //  *
  //  * @param index address to read from
  //  * @param value holds the read value if read is successful
  //  * @return Status::kOK if successful
  //  */
  // virtual Status read(int index, uint8_t& value) = 0;

  // /**
  //  * @brief Read a uint16_t value from the buffer
  //  *
  //  * @param index address to read from
  //  * @param value holds the read value if read is successful
  //  * @return Status::kOK if successful
  //  */
  // virtual Status read(int index, uint16_t& value) = 0;

  // /**
  //  * @brief Read a uint32_t value from the buffer
  //  *
  //  * @param index address to read from
  //  * @param value holds the read value if read is successful
  //  * @return Status::kOK if successful
  //  */
  // virtual Status read(int index, uint32_t& value) = 0;

  // /**
  //  * @brief Read a uint64_t value from the buffer
  //  *
  //  * @param index address to read from
  //  * @param value holds the read value if read is successful
  //  * @return Status::kOK if successful
  //  */
  // virtual Status read(int index, uint64_t& value) = 0;

  // /**
  //  * @brief Read a int8_t value from the buffer
  //  *
  //  * @param index address to read from
  //  * @param value holds the read value if read is successful
  //  * @return Status::kOK if successful
  //  */
  // virtual Status read(int index, int8_t& value) = 0;

  // /**
  //  * @brief Read a int16_t value from the buffer
  //  *
  //  * @param index address to read from
  //  * @param value holds the read value if read is successful
  //  * @return Status::kOK if successful
  //  */
  // virtual Status read(int index, int16_t& value) = 0;

  // /**
  //  * @brief Read a int32_t value from the buffer
  //  *
  //  * @param index address to read from
  //  * @param value holds the read value if read is successful
  //  * @return Status::kOK if successful
  //  */
  // virtual Status read(int index, int32_t& value) = 0;

  // /**
  //  * @brief Read a int64_t value from the buffer
  //  *
  //  * @param index address to read from
  //  * @param value holds the read value if read is successful
  //  * @return Status::kOK if successful
  //  */
  // virtual Status read(int index, int64_t& value) = 0;

  // /**
  //  * @brief Read a float value from the buffer
  //  *
  //  * @param index address to read from
  //  * @param value holds the read value if read is successful
  //  * @return Status::kOK if successful
  //  */
  // virtual Status read(int index, float& value) = 0;

  // /**
  //  * @brief Read a double value from the buffer
  //  *
  //  * @param index address to read from
  //  * @param value holds the read value if read is successful
  //  * @return Status::kOK if successful
  //  */
  // virtual Status read(int index, double& value) = 0;
};

}  // namespace proteus
#endif  // GUARD_PROTEUS_BUFFERS_BUFFER
