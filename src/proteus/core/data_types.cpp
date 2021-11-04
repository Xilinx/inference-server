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

#include "proteus/core/data_types.hpp"

#include <cstddef>    // for size_t
#include <stdexcept>  // for invalid_argument

#include "proteus/build_options.hpp"

#ifdef PROTEUS_ENABLE_VITIS
#include <xir/util/data_type.hpp>  // for DataType, DataType::FLOAT, DataTyp...
#endif

namespace proteus::types {

#ifdef PROTEUS_ENABLE_VITIS
DataType mapXirType(xir::DataType type) {
  auto data_type = type.type;
  size_t width = type.bit_width >> 3;  // bit -> byte
  if (data_type == xir::DataType::FLOAT) {
    if (width == getSize(DataType::FP32)) {
      return DataType::FP32;
    }
    if (width == getSize(DataType::FP64)) {
      return DataType::FP64;
    }
    throw std::invalid_argument("Unsupported float width: " +
                                std::to_string(width));
  }
  if (data_type == xir::DataType::INT || data_type == xir::DataType::XINT) {
    if (width == getSize(DataType::INT8)) {
      return DataType::INT8;
    }
    if (width == getSize(DataType::INT16)) {
      return DataType::INT16;
    }
    if (width == getSize(DataType::INT32)) {
      return DataType::INT32;
    }
    if (width == getSize(DataType::INT64)) {
      return DataType::INT64;
    }
    throw std::invalid_argument("Unsupported int width: " +
                                std::to_string(width));
  }
  if (data_type == xir::DataType::UINT || data_type == xir::DataType::XUINT) {
    if (width == getSize(DataType::UINT8)) {
      return DataType::UINT8;
    }
    if (width == getSize(DataType::UINT16)) {
      return DataType::UINT16;
    }
    if (width == getSize(DataType::UINT32)) {
      return DataType::UINT32;
    }
    if (width == getSize(DataType::UINT64)) {
      return DataType::UINT64;
      throw std::invalid_argument("Unsupported uint width: " +
                                  std::to_string(width));
    }
  }
  throw std::invalid_argument("Unsupported type: " + std::to_string(data_type));
}

xir::DataType mapTypeToXir(DataType type) {
  xir::DataType retval;
  switch (type) {
    case DataType::BOOL:
      retval.type = xir::DataType::UINT;
      retval.bit_width = getSize(DataType::BOOL) * 8;
      break;
    case DataType::UINT8:
      retval.type = xir::DataType::UINT;
      retval.bit_width = getSize(DataType::UINT8) * 8;
      break;
    case DataType::UINT16:
      retval.type = xir::DataType::UINT;
      retval.bit_width = getSize(DataType::UINT16) * 8;
      break;
    case DataType::UINT32:
      retval.type = xir::DataType::UINT;
      retval.bit_width = getSize(DataType::UINT32) * 8;
      break;
    case DataType::UINT64:
      retval.type = xir::DataType::UINT;
      retval.bit_width = getSize(DataType::UINT64) * 8;
      break;
    case DataType::INT8:
      retval.type = xir::DataType::INT;
      retval.bit_width = getSize(DataType::INT8) * 8;
      break;
    case DataType::INT16:
      retval.type = xir::DataType::INT;
      retval.bit_width = getSize(DataType::INT16) * 8;
      break;
    case DataType::INT32:
      retval.type = xir::DataType::INT;
      retval.bit_width = getSize(DataType::INT32) * 8;
      break;
    case DataType::INT64:
      retval.type = xir::DataType::INT;
      retval.bit_width = getSize(DataType::INT64) * 8;
      break;
    case DataType::FP32:
      retval.type = xir::DataType::FLOAT;
      retval.bit_width = getSize(DataType::FP32) * 8;
      break;
    case DataType::FP64:
      retval.type = xir::DataType::FLOAT;
      retval.bit_width = getSize(DataType::FP64) * 8;
      break;
    default:
      throw std::invalid_argument("Unsupported type conversion");
      break;
  }
  return retval;
}

#endif

std::string mapTypeToStr(DataType type) {
  switch (type) {
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
      return "";
  }
}

}  // namespace proteus::types
