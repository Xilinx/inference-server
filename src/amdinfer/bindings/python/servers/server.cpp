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
 * @brief Implements the Python bindings for the server.hpp header
 */

#include "amdinfer/servers/server.hpp"

#include <pybind11/cast.h>            // for arg
#include <pybind11/pybind11.h>        // for class_, init
#include <pybind11/stl.h>             // IWYU pragma: keep
#include <pybind11/stl/filesystem.h>  // IWYU pragma: keep

#include "amdinfer/bindings/python/helpers/docstrings.hpp"

namespace py = pybind11;

namespace amdinfer {

void wrapServer(py::module_ &m) {
  py::class_<Server>(m, "Server")
    .def(py::init<>(), DOCS(Server, Server))
    .def("startHttp", &Server::startHttp, py::arg("port"),
         DOCS(Server, startHttp))
    .def("stopHttp", &Server::stopHttp, DOCS(Server, stopHttp))
    .def("startGrpc", &Server::startGrpc, py::arg("port"),
         DOCS(Server, startGrpc))
    .def("stopGrpc", &Server::stopGrpc, DOCS(Server, stopGrpc))
    .def_static("setModelRepository", &Server::setModelRepository,
                py::arg("path"), DOCS(Server, setModelRepository))
    .def_static("enableRepositoryMonitoring",
                &Server::enableRepositoryMonitoring, py::arg("use_polling"),
                DOCS(Server, enableRepositoryMonitoring));
}

}  // namespace amdinfer
