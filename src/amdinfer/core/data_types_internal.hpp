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
 * @brief Defines the used data types internally
 */

#ifndef GUARD_AMDINFER_CORE_DATA_TYPES_INTERNAL
#define GUARD_AMDINFER_CORE_DATA_TYPES_INTERNAL

#include "amdinfer/build_options.hpp"    // for AMDINFER_ENABLE_VITIS
#include "amdinfer/core/data_types.hpp"  // for DataType

#ifdef AMDINFER_ENABLE_VITIS
namespace xir {
class DataType;
}  // namespace xir
#endif

namespace amdinfer {

#ifdef AMDINFER_ENABLE_VITIS
/**
 * @brief Given an XIR type, return the corresponding AMDinfer type
 *
 * @param type XIR datatype
 * @return DataType
 */
DataType mapXirToType(xir::DataType type);

/**
 * @brief Given a AMDinfer type, return the corresponding XIR type, if it exists
 *
 * @param type Datatype
 * @return xir::DataType
 */
xir::DataType mapTypeToXir(DataType type);
#endif

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_CORE_DATA_TYPES_INTERNAL
