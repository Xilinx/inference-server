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

#include <cstddef>   // for size_t
#include <cstdint>   // for int16_t, int32_t, int64_t, int8_t, uint16_t, uin...
#include <iostream>  // for ostream
#include <string>    // for string

#include "proteus/build_options.hpp"

#ifdef PROTEUS_ENABLE_VITIS
namespace xir {
class DataType;
}  // namespace xir
#endif

/**
 * @brief The types namespace defines the datatypes that Proteus supports in
 * incoming inference requests
 */
namespace proteus::types {

/**
 * @brief Data types supported in Proteus
 */
enum class DataType {
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
  FP64,
  STRING,
};

/// size of FP16 in bytes
constexpr auto kFp16Size = 2U;

/**
 * @brief Get the size in bytes associated with a data type
 *
 * @param type
 * @return constexpr size_t
 */
constexpr size_t getSize(DataType type) {
  switch (type) {
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
      return 0;
  }
}

/**
 * @brief Given a Proteus type, return a string corresponding to the type
 *
 * @param type DataType to convert
 * @return std::string
 */
std::string mapTypeToStr(DataType type);

std::ostream& operator<<(std::ostream& os, const DataType& bar);

#ifdef PROTEUS_ENABLE_VITIS
/**
 * @brief Given an XIR type, return the corresponding Proteus type
 *
 * @param type XIR datatype
 * @return DataType
 */
DataType mapXirType(xir::DataType type);

/**
 * @brief Given a Proteus type, return the corresponding XIR type, if it exists
 *
 * @param type Proteus datatype
 * @return xir::DataType
 */
xir::DataType mapTypeToXir(DataType type);
#endif

}  // namespace proteus::types
#endif  // GUARD_PROTEUS_CORE_DATA_TYPES
