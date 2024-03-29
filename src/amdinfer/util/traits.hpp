// Copyright 2022 Xilinx, Inc.
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

#ifndef GUARD_AMDINFER_UTIL_TRAITS
#define GUARD_AMDINFER_UTIL_TRAITS

#include <type_traits>

namespace amdinfer::util {

/**
 * @brief Similar to std::is_same, returns true if T is any of the types in the
 * variadic expression
 *
 * @tparam T base type to check
 * @tparam Ts list of types to compare against
 */
template <typename T, typename... Ts>
// NOLINTNEXTLINE(readability-identifier-naming)
struct is_any : std::disjunction<std::is_same<T, Ts>...> {};

template <typename T, typename... Ts>
// NOLINTNEXTLINE(readability-identifier-naming)
inline constexpr bool is_any_v = is_any<T, Ts...>::value;

}  // namespace amdinfer::util

#endif  // GUARD_AMDINFER_UTIL_TRAITS
