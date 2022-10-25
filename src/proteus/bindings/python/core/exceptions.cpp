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
 * @brief Implements the Python bindings for the exceptions
 */

#include "proteus/core/exceptions.hpp"

#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace proteus {

void wrapExceptions(py::module_& m) {
  py::register_exception<runtime_error>(m, "RuntimeError");
  py::register_exception<bad_status>(m, "BadStatus");
  py::register_exception<connection_error>(m, "ConnectionError");
  py::register_exception<file_not_found_error>(m, "FileNotFoundError");
  py::register_exception<file_read_error>(m, "FileReadError");
  py::register_exception<external_error>(m, "ExternalError");
  py::register_exception<invalid_argument>(m, "InvalidArgumentError");
  py::register_exception<environment_not_set_error>(m,
                                                    "EnvironmentNotSetError");
}

}  // namespace proteus
