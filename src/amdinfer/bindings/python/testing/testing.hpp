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
 * @brief Defines the Python bindings for the testing headers
 */

#ifndef GUARD_AMDINFER_BINDINGS_PYTHON_TESTING_TESTING
#define GUARD_AMDINFER_BINDINGS_PYTHON_TESTING_TESTING

namespace pybind11 {
class module_;
}  // namespace pybind11

void wrapTesting(pybind11::module_ &);

#endif  // GUARD_AMDINFER_BINDINGS_PYTHON_TESTING_TESTING
