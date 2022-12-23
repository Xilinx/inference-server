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
 * @brief Implements the Python bindings for the server.hpp header
 */

#include <pybind11/cast.h>      // for arg
#include <pybind11/pybind11.h>  // for class_, init
#include <pybind11/stl.h>       // IWYU pragma: keep

#include "amdinfer/testing/get_path_to_asset.hpp"

namespace py = pybind11;

void wrapTesting(py::module_ &m) {
  m.def("getPathToAsset", amdinfer::getPathToAsset, py::arg("key"));
}
