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
 * @brief Defines the objects for Python bindings for the core
 */

#ifndef GUARD_AMDINFER_BINDINGS_PYTHON_CORE_CORE
#define GUARD_AMDINFER_BINDINGS_PYTHON_CORE_CORE

namespace pybind11 {
class module_;
}  // namespace pybind11

namespace amdinfer {

void wrapDataType(pybind11::module_ &);
void wrapRequestParameters(pybind11::module_ &);
void wrapPredictApi(pybind11::module_ &);
void wrapExceptions(pybind11::module_ &);

void wrapCore(pybind11::module_ &m) {
  wrapExceptions(m);
  wrapDataType(m);
  wrapRequestParameters(m);
  wrapPredictApi(m);
}

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_BINDINGS_PYTHON_CORE_CORE
