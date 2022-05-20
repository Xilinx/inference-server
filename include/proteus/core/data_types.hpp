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
 * @brief Defines the data types used in Proteus
 */

#ifndef GUARD_PROTEUS_CORE_DATA_TYPES
#define GUARD_PROTEUS_CORE_DATA_TYPES

#include <cstddef>                    // for size_t
#include <cstdint>                    // for uint8_t, int16_t, int32_t, int64_t
#include <iostream>                   // for ostream
#include <stdexcept>                  // for invalid_argument
#include <string>                     // for string

#include "proteus/build_options.hpp"  // for PROTEUS_ENABLE_VITIS

#ifdef PROTEUS_ENABLE_VITIS
namespace xir {
class DataType;
}  // namespace xir
#endif

namespace proteus {

namespace detail {
// taken from https://stackoverflow.com/a/46711735
// used for hashing strings for switch statements
constexpr unsigned int hash(const char* s, int off = 0) {
  // NOLINTNEXTLINE
  return !s[off] ? 5381 : (hash(s, off + 1) * 33) ^ s[off];
}
}  // namespace detail

/**
 * @brief Data types supported in Proteus
 */
class DataType {
 public:
  enum Value : uint8_t {
    BOOL,
    UINT8,
    UINT16,
    UINT32,
    UINT64,
    INT8,
    INT16,
    INT32,
    INT64,
    FP16,
    FP32,
    FLOAT32 = FP32,
    FP64,
    FLOAT64 = FP64,
    STRING,
    UNKNOWN
  };

  constexpr DataType() : value_(Value::UNKNOWN) {}
  constexpr DataType(const char* value) : value_(mapStrToType(value)) {}
  constexpr DataType(DataType::Value value) : value_(value) {}

  constexpr operator Value() const { return value_; }

  friend std::ostream& operator<<(std::ostream& os, const DataType& value);

  /**
   * @brief Get the size in bytes associated with a data type
   *
   * @return constexpr size_t
   */
  constexpr size_t size() const {
    /// size of FP16 in bytes
    constexpr auto kFp16Size = 2U;

    switch (value_) {
      case DataType::BOOL:
        return sizeof(bool);
      case DataType::UINT8:
        return sizeof(uint8_t);
      case DataType::UINT16:
        return sizeof(uint16_t);
      case DataType::UINT32:
        return sizeof(uint32_t);
      case DataType::UINT64:
        return sizeof(uint64_t);
      case DataType::INT8:
        return sizeof(int8_t);
      case DataType::INT16:
        return sizeof(int16_t);
      case DataType::INT32:
        return sizeof(int32_t);
      case DataType::INT64:
        return sizeof(int64_t);
      case DataType::FP16:
        return kFp16Size;
      case DataType::FP32:
        return sizeof(float);
      case DataType::FP64:
        return sizeof(double);
      case DataType::STRING:
        return sizeof(std::string);
      default:
        throw std::invalid_argument("Unsupported type");
    }
  }

  /**
   * @brief Given a type, return a string corresponding to the type
   *
   * @return const char*
   */
  constexpr const char* str() const {
    switch (value_) {
      case DataType::BOOL:
        return "BOOL";
      case DataType::UINT8:
        return "UINT8";
      case DataType::UINT16:
        return "UINT16";
      case DataType::UINT32:
        return "UINT32";
      case DataType::UINT64:
        return "UINT64";
      case DataType::INT8:
        return "INT8";
      case DataType::INT16:
        return "INT16";
      case DataType::INT32:
        return "INT32";
      case DataType::INT64:
        return "INT64";
      case DataType::FP16:
        return "FP16";
      case DataType::FP32:
        return "FP32";
      case DataType::FP64:
        return "FP64";
      case DataType::STRING:
        return "STRING";
      default:
        throw std::invalid_argument("Unsupported type");
    }
  }

 private:
  constexpr DataType::Value mapStrToType(const char* value) const {
    switch (detail::hash(value)) {
      case detail::hash("BOOL"):
        return DataType::BOOL;
      case detail::hash("UINT8"):
        return DataType::UINT8;
      case detail::hash("UINT16"):
        return DataType::UINT16;
      case detail::hash("UINT32"):
        return DataType::UINT32;
      case detail::hash("UINT64"):
        return DataType::UINT64;
      case detail::hash("INT8"):
        return DataType::INT8;
      case detail::hash("INT16"):
        return DataType::INT16;
      case detail::hash("INT32"):
        return DataType::INT32;
      case detail::hash("INT64"):
        return DataType::INT64;
      case detail::hash("FP16"):
        return DataType::FP16;
      case detail::hash("FP32"):
        return DataType::FP32;
      case detail::hash("FP64"):
        return DataType::FP64;
      case detail::hash("STRING"):
        return DataType::STRING;
      case detail::hash("UNKNOWN"):
        return DataType::UNKNOWN;
      default:
        throw std::invalid_argument("Unsupported type construction");
    }
  }

  Value value_;
};

#ifdef PROTEUS_ENABLE_VITIS
/**
 * @brief Given an XIR type, return the corresponding Proteus type
 *
 * @param type XIR datatype
 * @return DataType
 */
DataType mapXirToType(xir::DataType type);

/**
 * @brief Given a Proteus type, return the corresponding XIR type, if it exists
 *
 * @param type Datatype
 * @return xir::DataType
 */
xir::DataType mapTypeToXir(DataType type);
#endif

}  // namespace proteus
#endif  // GUARD_PROTEUS_CORE_DATA_TYPES
