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
 * @brief Implements the Python bindings for the model_metadata.hpp header
 */

#include "amdinfer/core/model_metadata.hpp"

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

namespace py = pybind11;

namespace amdinfer {

void wrapModelMetadata(py::module_ &m) {
  py::class_<ModelMetadataTensor>(m, "ModelMetadataTensor")
    .def(py::init<const std::string &, amdinfer::DataType,
                  std::vector<uint64_t>>(),
         DOCS(ModelMetadataTensor, ModelMetadataTensor))
    .def("getName", &ModelMetadataTensor::getName,
         DOCS(ModelMetadataTensor, getName))
    .def("getDataType", &ModelMetadataTensor::getDataType,
         DOCS(ModelMetadataTensor, getDataType))
    .def("getShape", &ModelMetadataTensor::getShape,
         DOCS(ModelMetadataTensor, getShape));

  // NOLINTNEXTLINE(readability-identifier-naming)
  auto addInputTensor = static_cast<void (ModelMetadata::*)(
    const std::string &, amdinfer::DataType, std::vector<int>)>(
    &ModelMetadata::addInputTensor);
  // NOLINTNEXTLINE(readability-identifier-naming)
  auto addOutputTensor = static_cast<void (ModelMetadata::*)(
    const std::string &, amdinfer::DataType, std::vector<int>)>(
    &ModelMetadata::addOutputTensor);
  py::class_<ModelMetadata>(m, "ModelMetadata")
    .def(py::init<const std::string &, const std::string &>(),
         DOCS(ModelMetadata, ModelMetadata))
    .def("addInputTensor", addInputTensor, KeepAliveAssign(),
         DOCS(ModelMetadata, addInputTensor))
    .def("addOutputTensor", addOutputTensor, py::arg("name"),
         py::arg("datatype"), py::arg("shape"), KeepAliveAssign(),
         DOCS(ModelMetadata, addOutputTensor))
    .def_property("name", &ModelMetadata::getName, &ModelMetadata::setName)
    .def("getPlatform", &ModelMetadata::getPlatform,
         DOCS(ModelMetadata, getPlatform))
    .def("setReady", &ModelMetadata::setReady, DOCS(ModelMetadata, setReady))
    .def("isReady", &ModelMetadata::isReady, DOCS(ModelMetadata, isReady));
}

}  // namespace amdinfer
