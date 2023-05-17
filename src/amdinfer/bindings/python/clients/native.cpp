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

#include "amdinfer/clients/native.hpp"

#include <pybind11/cast.h>      // for arg
#include <pybind11/pybind11.h>  // for class_, init
#include <pybind11/stl.h>       // IWYU pragma: keep

#include "amdinfer/bindings/python/helpers/docstrings.hpp"  // for DOCS
#include "amdinfer/core/inference_request.hpp"   // for InferenceRequest
#include "amdinfer/core/inference_response.hpp"  // for InferenceResponse
#include "amdinfer/core/parameters.hpp"
#include "amdinfer/servers/server.hpp"  // for Server

namespace py = pybind11;

namespace amdinfer {

void wrapNativeClient(py::module_ &m) {
  py::class_<NativeClient, amdinfer::Client>(m, "NativeClient")
    .def(py::init<Server *>(), py::arg("server"))
    .def("serverMetadata", &NativeClient::serverMetadata,
         DOCS(NativeClient, serverMetadata))
    .def("serverLive", &NativeClient::serverLive,
         DOCS(NativeClient, serverLive))
    .def("serverReady", &NativeClient::serverReady,
         DOCS(NativeClient, serverReady))
    .def("workerLoad", &NativeClient::workerLoad, py::arg("model"),
         py::arg("parameters") = ParameterMap(), DOCS(NativeClient, workerLoad))
    .def("workerUnload", &NativeClient::workerUnload, py::arg("model"),
         DOCS(NativeClient, workerUnload))
    .def("modelList", &NativeClient::modelList, DOCS(NativeClient, modelList))
    .def("hasHardware", &NativeClient::hasHardware, py::arg("name"),
         py::arg("num"), DOCS(NativeClient, hasHardware));
}

}  // namespace amdinfer
