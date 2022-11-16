// Copyright 2021 Xilinx, Inc.
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
 * @brief Implements the type-related methods
 */

#include "amdinfer/core/data_types.hpp"

#include <cstddef>  // for size_t

#include "amdinfer/build_options.hpp"
#include "amdinfer/core/exceptions.hpp"

#ifdef AMDINFER_ENABLE_VITIS
#include <xir/util/data_type.hpp>  // for DataType, DataType::FLOAT, DataTyp...
#endif

namespace amdinfer {

#ifdef AMDINFER_ENABLE_VITIS
DataType mapXirToType(xir::DataType type) {
  auto data_type = type.type;
  size_t width = type.bit_width >> 3;  // bit -> byte
  if (data_type == xir::DataType::FLOAT) {
    if (width == DataType("FP32").size()) {
      return DataType::FP32;
    }
    if (width == DataType("FP64").size()) {
      return DataType::FP64;
    }
    throw invalid_argument("Unsupported XIR float width: " +
                           std::to_string(width));
  }
  if (data_type == xir::DataType::INT || data_type == xir::DataType::XINT) {
    if (width == DataType("INT8").size()) {
      return DataType::INT8;
    }
    if (width == DataType("INT16").size()) {
      return DataType::INT16;
    }
    if (width == DataType("INT32").size()) {
      return DataType::INT32;
    }
    if (width == DataType("INT64").size()) {
      return DataType::INT64;
    }
    throw invalid_argument("Unsupported XIR int width: " +
                           std::to_string(width));
  }
  if (data_type == xir::DataType::UINT || data_type == xir::DataType::XUINT) {
    if (width == DataType("UINT8").size()) {
      return DataType::UINT8;
    }
    if (width == DataType("UINT16").size()) {
      return DataType::UINT16;
    }
    if (width == DataType("UINT32").size()) {
      return DataType::UINT32;
    }
    if (width == DataType("UINT64").size()) {
      return DataType::UINT64;
    }
    throw invalid_argument("Unsupported XIR uint width: " +
                           std::to_string(width));
  }
  throw invalid_argument("Unsupported XIR type: " + std::to_string(data_type));
}

xir::DataType mapTypeToXir(DataType type) {
  xir::DataType retval;
  auto bit_width = static_cast<int32_t>(type.size()) * 8;
  switch (type) {
    case DataType::BOOL:
    case DataType::UINT8:
    case DataType::UINT16:
    case DataType::UINT32:
    case DataType::UINT64:
      retval.type = xir::DataType::UINT;
      break;
    case DataType::INT8:
    case DataType::INT16:
    case DataType::INT32:
    case DataType::INT64:
      retval.type = xir::DataType::INT;
      break;
    // case DataType::FP16 fall through to default handler
    case DataType::FP32:
    case DataType::FP64:
      retval.type = xir::DataType::FLOAT;
      break;
    default:
      throw invalid_argument("Unsupported type conversion to XIR");
  }
  retval.bit_width = bit_width;
  return retval;
}

#endif

std::ostream& operator<<(std::ostream& os, const DataType& value) {
  return os << value.str();
}

}  // namespace amdinfer
