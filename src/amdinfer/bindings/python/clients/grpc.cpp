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

#include "amdinfer/clients/grpc.hpp"

#include <pybind11/cast.h>      // for arg
#include <pybind11/pybind11.h>  // for class_, init
#include <pybind11/stl.h>       // IWYU pragma: keep

#include "amdinfer/bindings/python/helpers/docstrings.hpp"  // for DOCS
#include "amdinfer/core/inference_request.hpp"   // for InferenceRequest
#include "amdinfer/core/inference_response.hpp"  // for InferenceResponse
#include "amdinfer/core/parameters.hpp"

namespace py = pybind11;

namespace amdinfer {

class ParameterMap;

void wrapGrpcClient(py::module_ &m) {
  py::class_<GrpcClient, amdinfer::Client>(m, "GrpcClient")
    .def(py::init<const std::string &>(), py::arg("address"),
         DOCS(GrpcClient, GrpcClient))
    .def("serverMetadata", &GrpcClient::serverMetadata,
         DOCS(GrpcClient, serverMetadata))
    .def("serverLive", &GrpcClient::serverLive, DOCS(GrpcClient, serverLive))
    .def("serverReady", &GrpcClient::serverReady, DOCS(GrpcClient, serverReady))
    .def("workerLoad", &GrpcClient::workerLoad, py::arg("model"),
         py::arg("parameters") = ParameterMap(), DOCS(GrpcClient, workerLoad))
    .def("workerUnload", &GrpcClient::workerUnload, py::arg("model"),
         DOCS(GrpcClient, workerUnload))
    .def("modelList", &GrpcClient::modelList, DOCS(GrpcClient, modelList))
    .def("hasHardware", &GrpcClient::hasHardware, py::arg("name"),
         py::arg("num"), DOCS(GrpcClient, hasHardware));
}

}  // namespace amdinfer
