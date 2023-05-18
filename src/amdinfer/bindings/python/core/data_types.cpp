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
 * @brief Implements the Python bindings for the data_types.hpp header
 */

#include "amdinfer/core/data_types.hpp"

#include <pybind11/operators.h>  // for self, operator==, self_t
#include <pybind11/pybind11.h>   // for class_, enum_, object, init, module_

#include <sstream>  // IWYU pragma: keep

namespace py = pybind11;

namespace amdinfer {

void wrapDataType(py::module_& m) {
  auto datatype = py::class_<DataType>(m, "DataType");
  auto value = py::enum_<DataType::Value>(datatype, "Value");

  datatype.def(py::init<>())
    .def(py::init<>())
    .def(py::init<const char*>())
    .def(py::init<DataType::Value>())
    .def_property_readonly_static(
      "BOOL", [](const py::object& /*self*/) { return DataType("BOOL"); })
    .def_property_readonly_static(
      "UINT8", [](const py::object& /*self*/) { return DataType("UINT8"); })
    .def_property_readonly_static(
      "UINT16", [](const py::object& /*self*/) { return DataType("UINT16"); })
    .def_property_readonly_static(
      "UINT32", [](const py::object& /*self*/) { return DataType("UINT32"); })
    .def_property_readonly_static(
      "UINT64", [](const py::object& /*self*/) { return DataType("UINT64"); })
    .def_property_readonly_static(
      "INT8", [](const py::object& /*self*/) { return DataType("INT8"); })
    .def_property_readonly_static(
      "INT16", [](const py::object& /*self*/) { return DataType("INT16"); })
    .def_property_readonly_static(
      "INT32", [](const py::object& /*self*/) { return DataType("INT32"); })
    .def_property_readonly_static(
      "INT64", [](const py::object& /*self*/) { return DataType("INT64"); })
    .def_property_readonly_static(
      "FP16", [](const py::object& /*self*/) { return DataType("FP16"); })
    .def_property_readonly_static(
      "FP32", [](const py::object& /*self*/) { return DataType("FP32"); })
    .def_property_readonly_static(
      "FLOAT32", [](const py::object& /*self*/) { return DataType("FP32"); })
    .def_property_readonly_static(
      "FP64", [](const py::object& /*self*/) { return DataType("FP64"); })
    .def_property_readonly_static(
      "FLOAT64", [](const py::object& /*self*/) { return DataType("FP64"); })
    .def_property_readonly_static(
      "BYTES", [](const py::object& /*self*/) { return DataType("BYTES"); })
    .def("size", &DataType::size)
    .def("str", &DataType::str)
    // defines the __eq__ operator for the class
    .def(py::self == py::self)  // NOLINT(misc-redundant-expression)
    .def("__repr__",
         [](const DataType& self) {
           return "DataType(" + std::string(self.str()) + ")\n";
         })
    .def("__str__", [](const DataType& self) {
      std::ostringstream os;
      os << self;
      return os.str();
    });

  value.value("BOOL", DataType::Bool)
    .value("UINT8", DataType::Uint8)
    .value("UINT16", DataType::Uint16)
    .value("UINT32", DataType::Uint32)
    .value("UINT64", DataType::Uint64)
    .value("INT8", DataType::Int8)
    .value("INT16", DataType::Int16)
    .value("INT32", DataType::Int32)
    .value("INT64", DataType::Int64)
    .value("FP16", DataType::Fp16)
    .value("FP32", DataType::Fp32)
    .value("FLOAT32", DataType::Fp32)
    .value("FP64", DataType::Fp64)
    .value("FLOAT64", DataType::Fp64)
    .value("BYTES", DataType::Bytes);

  py::implicitly_convertible<DataType::Value, DataType>();
}

}  // namespace amdinfer
