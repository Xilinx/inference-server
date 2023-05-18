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

/**
 * @file
 * @brief Implements the Python bindings for the http.hpp header
 */

#include "amdinfer/clients/http.hpp"

#include <pybind11/cast.h>      // for arg
#include <pybind11/pybind11.h>  // for class_, init
#include <pybind11/stl.h>       // IWYU pragma: keep

#include <unordered_map>  // for unordered_map

#include "amdinfer/bindings/python/helpers/docstrings.hpp"  // for DOCS
#include "amdinfer/core/inference_request.hpp"   // for InferenceRequest
#include "amdinfer/core/inference_response.hpp"  // for InferenceResponse
#include "amdinfer/core/parameters.hpp"

namespace py = pybind11;

namespace amdinfer {

class ParameterMap;

void wrapHttpClient(py::module_ &m) {
  using amdinfer::HttpClient;

  // default number of clients to create for parallelism
  const int parallelism = 32;

  py::class_<HttpClient, amdinfer::Client>(m, "HttpClient")
    .def(py::init<const std::string &,
                  const std::unordered_map<std::string, std::string>, int>(),
         py::arg("address"),
         py::arg("headers") = std::unordered_map<std::string, std::string>(),
         py::arg("parallelism") = parallelism, DOCS(HttpClient, HttpClient))
    .def("serverMetadata", &HttpClient::serverMetadata,
         DOCS(HttpClient, serverMetadata))
    .def("serverLive", &HttpClient::serverLive, DOCS(HttpClient, serverLive))
    .def("serverReady", &HttpClient::serverReady, DOCS(HttpClient, serverReady))
    .def("workerLoad", &HttpClient::workerLoad, py::arg("model"),
         py::arg("parameters") = ParameterMap(), DOCS(HttpClient, workerLoad))
    .def("workerUnload", &HttpClient::workerUnload, py::arg("model"),
         DOCS(HttpClient, workerUnload))
    .def("modelList", &HttpClient::modelList, DOCS(HttpClient, modelList))
    .def("hasHardware", &HttpClient::hasHardware, py::arg("name"),
         py::arg("num"), DOCS(HttpClient, hasHardware));
}

}  // namespace amdinfer
