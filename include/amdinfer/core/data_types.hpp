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
 * @brief Defines the used data types
 */

#ifndef GUARD_AMDINFER_CORE_DATA_TYPES
#define GUARD_AMDINFER_CORE_DATA_TYPES

#include <cassert>   // for assert
#include <cstddef>   // for size_t
#include <cstdint>   // for uint8_t, int16_t, int32_t
#include <iostream>  // for ostream
#include <string>    // for string

#include "amdinfer/core/exceptions.hpp"  // for invalid_argument
#include "half/half.hpp"                 // for half

namespace amdinfer {

namespace detail {
// taken from https://stackoverflow.com/a/46711735
// used for hashing strings for switch statements
constexpr unsigned int hash(std::string_view str) {
  const int initial_hash = 5381;
  const int shift = 5;

  const auto* const data = str.data();
  const auto size = str.size();

  uint32_t hash = initial_hash;
  for (const char* c = data; c < data + size; ++c) {
    hash = ((hash << shift) + hash) + static_cast<int>(*c);
  }
  return hash;
}
}  // namespace detail

// this is kept lower-case for visual consistency with other POD types
using fp16 = half_float::half;  // NOLINT(readability-identifier-naming)

/**
 * @brief Supported data types. The ALL_CAPS aliases are deprecated and will
 * be removed.
 */
class DataType {
 public:
  enum Value : uint8_t {
    Bool,
    BOOL = Bool,
    Uint8,
    UINT8 = Uint8,
    Uint16,
    UINT16 = Uint16,
    Uint32,
    UINT32 = Uint32,
    Uint64,
    UINT64 = Uint64,
    Int8,
    INT8 = Int8,
    Int16,
    INT16 = Int16,
    Int32,
    INT32 = Int32,
    Int64,
    INT64 = Int64,
    Fp16,
    Float16 = Fp16,
    FP16 = Fp16,
    Fp32,
    Float32 = Fp32,
    FP32 = Fp32,
    Fp64,
    Float64 = Fp64,
    FP64 = Fp64,
    String,
    STRING = String,
    Unknown,
    UNKNOWN = Unknown
  };

  /// Constructs a new DataType object
  constexpr DataType() = default;
  /**
   * @brief Constructs a new DataType object
   *
   * @param value string to identify the initial value of the new datatype
   */
  constexpr explicit DataType(const char* value)
    : value_(mapStrToType(value)) {}
  /**
   * @brief Constructs a new DataType object
   *
   * @param value datatype to identify the initial value of the new datatype
   */
  // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
  constexpr DataType(DataType::Value value) : value_(value) {}

  /// Implicit conversion between the Datatype class and its internal value
  // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
  constexpr operator Value() const { return value_; }

  /**
   * @brief Print support for the DataType class
   *
   * @param os stream to print to
   * @param value Datatype instance to print
   * @return std::ostream&
   */
  friend std::ostream& operator<<(std::ostream& os, const DataType& value);

  /**
   * @brief Get the size in bytes associated with a data type
   *
   * @return constexpr size_t
   */
  [[nodiscard]] constexpr size_t size() const {
    switch (value_) {
      case DataType::Bool:
        return sizeof(bool);
      case DataType::Uint8:
        return sizeof(uint8_t);
      case DataType::Uint16:
        return sizeof(uint16_t);
      case DataType::Uint32:
        return sizeof(uint32_t);
      case DataType::Uint64:
        return sizeof(uint64_t);
      case DataType::Int8:
        return sizeof(int8_t);
      case DataType::Int16:
        return sizeof(int16_t);
      case DataType::Int32:
        return sizeof(int32_t);
      case DataType::Int64:
        return sizeof(int64_t);
      case DataType::Fp16:
        return sizeof(fp16);
      case DataType::Fp32:
        return sizeof(float);
      case DataType::Fp64:
        return sizeof(double);
      case DataType::String:
        return sizeof(std::string);
      default:
        throw invalid_argument("Unknown datatype passed");
    }
  }

  /**
   * @brief Given a type, return a string corresponding to the type. KServe
   * requires the string form to be specific values and in all caps. This
   * adheres to that. If these string values are changed, then each server will
   * need to map the values to the ones KServe expects.
   *
   * @return const char*
   */
  [[nodiscard]] constexpr const char* str() const {
    switch (value_) {
      case DataType::Bool:
        return "BOOL";
      case DataType::Uint8:
        return "UINT8";
      case DataType::Uint16:
        return "UINT16";
      case DataType::Uint32:
        return "UINT32";
      case DataType::Uint64:
        return "UINT64";
      case DataType::Int8:
        return "INT8";
      case DataType::Int16:
        return "INT16";
      case DataType::Int32:
        return "INT32";
      case DataType::Int64:
        return "INT64";
      case DataType::Fp16:
        return "FP16";
      case DataType::Fp32:
        return "FP32";
      case DataType::Fp64:
        return "FP64";
      case DataType::String:
        return "STRING";
      default:
        throw invalid_argument("Unknown datatype passed");
    }
  }

 private:
  constexpr DataType::Value static mapStrToType(const char* value) {
    switch (detail::hash(value)) {
      case detail::hash("BOOL"):
      case detail::hash("Bool"):
        return DataType::Bool;
      case detail::hash("UINT8"):
      case detail::hash("Uint8"):
        return DataType::Uint8;
      case detail::hash("UINT16"):
      case detail::hash("Uint16"):
        return DataType::Uint16;
      case detail::hash("UINT32"):
      case detail::hash("Uint32"):
        return DataType::Uint32;
      case detail::hash("UINT64"):
      case detail::hash("Uint64"):
        return DataType::Uint64;
      case detail::hash("INT8"):
      case detail::hash("Int8"):
        return DataType::Int8;
      case detail::hash("INT16"):
      case detail::hash("Int16"):
        return DataType::Int16;
      case detail::hash("INT32"):
      case detail::hash("Int32"):
        return DataType::Int32;
      case detail::hash("INT64"):
      case detail::hash("Int64"):
        return DataType::Int64;
      case detail::hash("FP16"):
      case detail::hash("Fp16"):
        return DataType::Fp16;
      case detail::hash("FP32"):
      case detail::hash("Fp32"):
        return DataType::Fp32;
      case detail::hash("FP64"):
      case detail::hash("Fp64"):
        return DataType::Fp64;
      case detail::hash("STRING"):
      case detail::hash("String"):
        return DataType::String;
      default:
        throw invalid_argument("Unknown datatype passed");
    }
  }

  Value value_ = Value::Unknown;
};

/**
 * @brief Runs a callable templated function based on the type passed as an
 * argument
 *
 * @tparam F a callable templated function type
 * @tparam Args the types of arguments to the function F
 * @param f a callable templated function to evaluate for all datatypes
 * @param type the datatype to choose which templated function f to run
 * @param args arguments to the function F
 * @return auto - return value from the function f
 */
template <typename F, typename... Args>
auto switchOverTypes(F f, DataType type, [[maybe_unused]] const Args&... args) {
  switch (type) {
    case DataType::Bool: {
      return f.template operator()<bool>(args...);
    }
    case DataType::Uint8: {
      return f.template operator()<uint8_t>(args...);
    }
    case DataType::Uint16: {
      return f.template operator()<uint16_t>(args...);
    }
    case DataType::Uint32: {
      return f.template operator()<uint32_t>(args...);
    }
    case DataType::Uint64: {
      return f.template operator()<uint64_t>(args...);
    }
    case DataType::Int8: {
      return f.template operator()<int8_t>(args...);
    }
    case DataType::Int16: {
      return f.template operator()<int16_t>(args...);
    }
    case DataType::Int32: {
      return f.template operator()<int32_t>(args...);
    }
    case DataType::Int64: {
      return f.template operator()<int64_t>(args...);
    }
    case DataType::Fp16: {
      return f.template operator()<fp16>(args...);
    }
    case DataType::Fp32: {
      return f.template operator()<float>(args...);
    }
    case DataType::Fp64: {
      return f.template operator()<double>(args...);
    }
    case DataType::String: {
      return f.template operator()<char>(args...);
    }
    default:
      throw invalid_argument("Unknown datatype passed");
  }
}

}  // namespace amdinfer
#endif  // GUARD_AMDINFER_CORE_DATA_TYPES
