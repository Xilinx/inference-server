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

#include <unordered_map>  // for unordered_map

#include "amdinfer/bindings/python/helpers/docstrings.hpp"  // for DOCS
#include "amdinfer/declarations.hpp"

namespace py = pybind11;

namespace amdinfer {

void wrapGrpcClient(py::module_ &m) {
  py::class_<GrpcClient, amdinfer::Client>(m, "GrpcClient")
    .def(py::init<const std::string &>(), py::arg("address"),
         DOCS(GrpcClient, GrpcClient))
    .def("serverMetadata", &GrpcClient::serverMetadata,
         DOCS(GrpcClient, serverMetadata))
    .def("serverLive", &GrpcClient::serverLive, DOCS(GrpcClient, serverLive))
    .def("serverReady", &GrpcClient::serverReady, DOCS(GrpcClient, serverReady))
    .def("modelReady", &GrpcClient::modelReady, py::arg("model"),
         DOCS(GrpcClient, modelReady))
    .def("modelMetadata", &GrpcClient::modelMetadata, py::arg("model"),
         DOCS(GrpcClient, modelMetadata))
    .def("modelLoad", &GrpcClient::modelLoad, py::arg("model"),
         py::arg("parameters") = static_cast<RequestParameters *>(nullptr),
         DOCS(GrpcClient, modelLoad))
    .def("modelUnload", &GrpcClient::modelUnload, py::arg("model"),
         DOCS(GrpcClient, modelUnload))
    .def("workerLoad", &GrpcClient::workerLoad, py::arg("model"),
         py::arg("parameters") = static_cast<RequestParameters *>(nullptr),
         DOCS(GrpcClient, workerLoad))
    .def("workerUnload", &GrpcClient::workerUnload, py::arg("model"),
         DOCS(GrpcClient, workerUnload))
    .def("modelInfer", &GrpcClient::modelInfer, py::arg("model"),
         py::arg("request"), DOCS(GrpcClient, modelInfer))
    // cannot wrap future directly in Python
    // .def("modelInferAsync", &GrpcClient::modelInferAsync, py::arg("model"),
    //      py::arg("request"), DOCS(GrpcClient, modelInferAsync))
    .def("modelList", &GrpcClient::modelList, DOCS(GrpcClient, modelList))
    .def("hasHardware", &GrpcClient::hasHardware, py::arg("name"),
         py::arg("num"), DOCS(GrpcClient, hasHardware));
}

}  // namespace amdinfer
