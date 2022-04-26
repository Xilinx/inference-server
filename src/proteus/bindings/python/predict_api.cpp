// Copyright 2022 Xilinx Inc.
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
 * @brief Implements the Python bindings for the predict_api.hpp header
 */

#include "proteus/core/predict_api.hpp"

#include <pybind11/pybind11.h>

#include "docstrings.hpp"

namespace py = pybind11;

using proteus::RequestParameters;

void initRequestParameters(py::module_ &m) {
  py::class_<RequestParameters>(m, "RequestParameters")
    .def(py::init<>(), DOC(proteus, RequestParameters))
    .def("put",
         py::overload_cast<const std::string &, bool>(&RequestParameters::put),
         DOC(proteus, RequestParameters, put))
    .def(
      "put",
      py::overload_cast<const std::string &, double>(&RequestParameters::put),
      DOC(proteus, RequestParameters, put, 2))
    .def(
      "put",
      py::overload_cast<const std::string &, int32_t>(&RequestParameters::put),
      DOC(proteus, RequestParameters, put, 3))
    .def("put",
         py::overload_cast<const std::string &, const std::string &>(
           &RequestParameters::put),
         DOC(proteus, RequestParameters, put, 4))
    .def("put",
         py::overload_cast<const std::string &, const char *>(
           &RequestParameters::put),
         DOC(proteus, RequestParameters, put, 5))
    .def("getBool", &RequestParameters::get<bool>,
         DOC(proteus, RequestParameters, get))
    .def("getFloat", &RequestParameters::get<double>,
         DOC(proteus, RequestParameters, get))
    .def("getInt", &RequestParameters::get<int32_t>,
         DOC(proteus, RequestParameters, get))
    .def("getString", &RequestParameters::get<std::string>,
         DOC(proteus, RequestParameters, get))
    .def("has", &RequestParameters::has, DOC(proteus, RequestParameters, has),
         py::arg("key"))
    .def("erase", &RequestParameters::erase,
         DOC(proteus, RequestParameters, erase))
    .def("empty", &RequestParameters::empty,
         DOC(proteus, RequestParameters, empty))
    .def(
      "__iter__",
      [](const RequestParameters &s) {
        return py::make_iterator(s.cbegin(), s.cend());
      },
      py::keep_alive<0, 1>());
}
