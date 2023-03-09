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

/**
 * @file
 * @brief Implements the type-related methods internally
 */

#include "amdinfer/core/data_types_internal.hpp"

#include <cstddef>  // for size_t
#include <cstdint>  // for int32_t
#include <string>   // for operator+, to_string

#include "amdinfer/core/exceptions.hpp"  // for invalid_argument

#ifdef AMDINFER_ENABLE_VITIS
#include <xir/util/data_type.hpp>  // for DataType, DataType::FLOAT, DataTyp...
#endif

#ifdef AMDINFER_ENABLE_VITIS

namespace amdinfer {

const auto kBitsInByte = 8;

DataType mapXirToType(xir::DataType type) {
  auto data_type = type.type;
  size_t width = type.bit_width / kBitsInByte;
  if (data_type == xir::DataType::FLOAT) {
    if (width == DataType("FP32").size()) {
      return DataType::Fp32;
    }
    if (width == DataType("FP64").size()) {
      return DataType::Fp64;
    }
    throw invalid_argument("Unsupported XIR float width: " +
                           std::to_string(width));
  }
  if (data_type == xir::DataType::INT || data_type == xir::DataType::XINT) {
    if (width == DataType("INT8").size()) {
      return DataType::Int8;
    }
    if (width == DataType("INT16").size()) {
      return DataType::Int16;
    }
    if (width == DataType("INT32").size()) {
      return DataType::Int32;
    }
    if (width == DataType("INT64").size()) {
      return DataType::Int64;
    }
    throw invalid_argument("Unsupported XIR int width: " +
                           std::to_string(width));
  }
  if (data_type == xir::DataType::UINT || data_type == xir::DataType::XUINT) {
    if (width == DataType("UINT8").size()) {
      return DataType::Uint8;
    }
    if (width == DataType("UINT16").size()) {
      return DataType::Uint16;
    }
    if (width == DataType("UINT32").size()) {
      return DataType::Uint32;
    }
    if (width == DataType("UINT64").size()) {
      return DataType::Uint64;
    }
    throw invalid_argument("Unsupported XIR uint width: " +
                           std::to_string(width));
  }
  throw invalid_argument("Unsupported XIR type: " + std::to_string(data_type));
}

xir::DataType mapTypeToXir(DataType type) {
  xir::DataType retval;
  auto bit_width = static_cast<int32_t>(type.size()) * kBitsInByte;
  switch (type) {
    case DataType::Bool:
    case DataType::Uint8:
    case DataType::Uint16:
    case DataType::Uint32:
    case DataType::Uint64:
      retval.type = xir::DataType::XUINT;
      break;
    case DataType::Int8:
    case DataType::Int16:
    case DataType::Int32:
    case DataType::Int64:
      retval.type = xir::DataType::XINT;
      break;
    // case DataType::Fp16 fall through to default handler
    case DataType::Fp32:
    case DataType::Fp64:
      retval.type = xir::DataType::FLOAT;
      break;
    default:
      throw invalid_argument("Unsupported type conversion to XIR");
  }
  retval.bit_width = bit_width;
  return retval;
}

}  // namespace amdinfer

#endif
