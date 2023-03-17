// Copyright 2023 Advanced Micro Devices, Inc.
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
 * @brief Implements the Python bindings for the inference_response.hpp header
 */

#include "amdinfer/core/inference_response.hpp"

#include <pybind11/attr.h>      // for keep_alive
#include <pybind11/cast.h>      // for arg
#include <pybind11/numpy.h>     // for array_t
#include <pybind11/pybind11.h>  // for class_, init
#include <pybind11/stl.h>       // IWYU pragma: keep

#include <array>          // for array
#include <cstring>        // for memcpy
#include <sstream>        // IWYU pragma: keep
#include <unordered_map>  // for unordered_map

#include "amdinfer/bindings/python/helpers/docstrings.hpp"  // for DOCS
#include "amdinfer/bindings/python/helpers/keep_alive.hpp"  // for keep_alive
#include "amdinfer/bindings/python/helpers/print.hpp"       // for toString
#include "amdinfer/core/inference_request.hpp"

namespace py = pybind11;

namespace amdinfer {

void wrapInferenceResponse(py::module_ &m) {
  py::class_<InferenceResponse>(m, "InferenceResponse")
    .def(py::init<>(), DOCS(InferenceResponse, InferenceResponse))
    .def(py::init<const std::string &>(),
         DOCS(InferenceResponse, InferenceResponse, 2))
    .def_property("id", &InferenceResponse::getID, &InferenceResponse::setID)
    .def_property("model", &InferenceResponse::getModel,
                  &InferenceResponse::setModel)
#ifdef AMDINFER_ENABLE_TRACING
    .def("getContext", &InferenceResponse::getContext)
    .def("setContext",
         [](InferenceResponse &self, amdinfer::StringMap context) {
           self.setContext(std::move(context));
         })
#endif
    .def("getParameters", &InferenceResponse::getParameters,
         DOCS(InferenceResponse, getParameters))
    .def("getOutputs", &InferenceResponse::getOutputs,
         py::return_value_policy::reference_internal,
         DOCS(InferenceResponse, getOutputs))
    .def("addOutput", &InferenceResponse::addOutput, py::arg("output"),
         KeepAliveAssign(), DOCS(InferenceResponse, addOutput))
    .def("isError", &InferenceResponse::isError,
         DOCS(InferenceResponse, isError))
    .def("getError", &InferenceResponse::getError,
         DOCS(InferenceResponse, getError))
    .def("__repr__",
         [](const InferenceResponse &self) {
           (void)self;
           return "InferenceResponse\n";
         })
    .def("__str__", &amdinfer::toString<InferenceResponse>);
}

void wrapInferenceResponses(py::module &m) { wrapInferenceResponse(m); }

}  // namespace amdinfer
