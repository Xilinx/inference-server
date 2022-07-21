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

#include "proteus/bindings/python/helpers/docstrings.hpp"

namespace py = pybind11;

void wrapHttpClient(py::module_ &m) {
  using proteus::HttpClient;

  py::class_<HttpClient, proteus::Client>(m, "HttpClient")
    .def(py::init<const std::string &,
                  const std::unordered_map<std::string, std::string>>(),
         py::arg("address"),
         py::arg("headers") = std::unordered_map<std::string, std::string>(),
         DOCS(HttpClient, HttpClient))
    .def("serverMetadata", &HttpClient::serverMetadata,
         DOCS(HttpClient, serverMetadata))
    .def("serverLive", &HttpClient::serverLive, DOCS(HttpClient, serverLive))
    .def("serverReady", &HttpClient::serverReady, DOCS(HttpClient, serverReady))
    .def("modelReady", &HttpClient::modelReady, py::arg("model"),
         DOCS(HttpClient, modelReady))
    .def("modelLoad", &HttpClient::modelLoad, py::arg("model"),
         py::arg("parameters") = proteus::RequestParameters(),
         DOCS(HttpClient, modelLoad))
    .def("modelUnload", &HttpClient::modelUnload, py::arg("model"),
         DOCS(HttpClient, modelUnload))
    .def("workerLoad", &HttpClient::workerLoad, py::arg("model"),
         py::arg("parameters") = proteus::RequestParameters(),
         DOCS(HttpClient, workerLoad))
    .def("workerUnload", &HttpClient::workerUnload, py::arg("model"),
         DOCS(HttpClient, workerUnload))
    .def("modelInfer", &HttpClient::modelInfer, py::arg("model"),
         py::arg("request"), DOCS(HttpClient, modelInfer))
    .def("modelList", &HttpClient::modelList, DOCS(HttpClient, modelList))
    .def("hasHardware", &HttpClient::hasHardware, py::arg("name"),
         py::arg("num"), DOCS(HttpClient, modelList));
}
