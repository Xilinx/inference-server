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
 * @brief Implements the Python bindings for the http.hpp header
 */

#include "proteus/clients/http.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sstream>

#include "docstrings.hpp"

namespace py = pybind11;

void wrapHttpClient(py::module_ &m) {
  using proteus::HttpClient;

  // auto foo = (py::object)
  // py::module_::import("proteus").attr("RequestParameters");
  // py::module_::import("proteus").attr("InferenceRequest");
  // py::module_::import("proteus").attr("InferenceResponse");

  // auto foo = (py::object) py::module_::import("clients").attr("Client");

  py::class_<HttpClient, proteus::Client>(m, "HttpClient")
    .def(py::init<const std::string &>(), py::arg("address"))
    .def("serverMetadata", &HttpClient::serverMetadata)
    .def("serverLive", &HttpClient::serverLive)
    .def("serverReady", &HttpClient::serverReady)
    .def("modelReady", &HttpClient::modelReady, py::arg("model"))
    .def("modelLoad", &HttpClient::modelLoad, py::arg("model"),
         py::arg("parameters") = proteus::RequestParameters())
    .def("modelUnload", &HttpClient::modelUnload, py::arg("model"))
    .def("modelInfer", &HttpClient::modelInfer, py::arg("model"),
         py::arg("request"))
    .def("modelList", &HttpClient::modelList);
}
