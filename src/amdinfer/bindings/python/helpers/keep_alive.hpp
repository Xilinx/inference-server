// Copyright 2023 Advanced Micro Devices, Inc.
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
 * @brief
 */

#ifndef GUARD_AMDINFER_BINDINGS_PYTHON_HELPERS_KEEP_ALIVE
#define GUARD_AMDINFER_BINDINGS_PYTHON_HELPERS_KEEP_ALIVE

#include <pybind11/pybind11.h>

namespace amdinfer {

// https://pybind11.readthedocs.io/en/stable/advanced/functions.html#keep-alive
constexpr auto kKeepAliveReturn = 0;
constexpr auto kKeepAliveSelf = 1;
constexpr auto kKeepAliveArg0 = 2;

using KeepAliveReturn = pybind11::keep_alive<kKeepAliveReturn, kKeepAliveSelf>;
using KeepAliveAssign = pybind11::keep_alive<kKeepAliveSelf, kKeepAliveArg0>;

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_BINDINGS_PYTHON_HELPERS_KEEP_ALIVE
