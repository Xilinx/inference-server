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
 * @brief Implements the Python bindings for the data_types.hpp header
 */

#include "proteus/core/data_types.hpp"

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include <sstream>

#ifdef PROTEUS_ENABLE_VITIS
#include <xir/util/data_type.hpp>
#endif

#include "proteus/bindings/python/helpers/docstrings.hpp"

namespace py = pybind11;

using proteus::DataType;

void wrapDataType(py::module_& m) {
  auto datatype = py::class_<DataType>(m, "DataType");
  auto value = py::enum_<DataType::Value>(datatype, "Value");

  datatype.def(py::init<>())
    .def(py::init<const char*>())
    .def(py::init<DataType::Value>())
    .def_property_readonly_static("BOOL",
                                  [](py::object) { return DataType("BOOL"); })
    .def_property_readonly_static("UINT8",
                                  [](py::object) { return DataType("UINT8"); })
    .def_property_readonly_static("UINT16",
                                  [](py::object) { return DataType("UINT16"); })
    .def_property_readonly_static("UINT32",
                                  [](py::object) { return DataType("UINT32"); })
    .def_property_readonly_static("UINT64",
                                  [](py::object) { return DataType("UINT64"); })
    .def_property_readonly_static("INT8",
                                  [](py::object) { return DataType("INT8"); })
    .def_property_readonly_static("INT16",
                                  [](py::object) { return DataType("INT16"); })
    .def_property_readonly_static("INT32",
                                  [](py::object) { return DataType("INT32"); })
    .def_property_readonly_static("INT64",
                                  [](py::object) { return DataType("INT64"); })
    .def_property_readonly_static("FP16",
                                  [](py::object) { return DataType("FP16"); })
    .def_property_readonly_static("FP32",
                                  [](py::object) { return DataType("FP32"); })
    .def_property_readonly_static("FP64",
                                  [](py::object) { return DataType("FP64"); })
    .def_property_readonly_static("STRING",
                                  [](py::object) { return DataType("STRING"); })
    .def("size", &DataType::size)
    .def("str", &DataType::str)
    .def(py::self == py::self)
    .def("__repr__",
         [](const DataType& self) {
           return "DataType(" + std::string(self.str()) + ")\n";
         })
    .def("__str__", [](const DataType& self) {
      std::ostringstream os;
      os << self;
      return os.str();
    });

  value.value("BOOL", DataType::BOOL)
    .value("UINT8", DataType::UINT8)
    .value("UINT16", DataType::UINT16)
    .value("UINT32", DataType::UINT32)
    .value("UINT64", DataType::UINT64)
    .value("INT8", DataType::INT8)
    .value("INT16", DataType::INT16)
    .value("INT32", DataType::INT32)
    .value("INT64", DataType::INT64)
    .value("FP16", DataType::FP16)
    .value("FP32", DataType::FP32)
    .value("FP64", DataType::FP64)
    .value("STRING", DataType::STRING);

  py::implicitly_convertible<DataType::Value, DataType>();
}

void wrapTypeMaps(py::module_& m) {
#ifdef PROTEUS_ENABLE_VITIS
  py::module_::import("xir").attr("DataType");

  m.def("mapXirType", proteus::mapXirToType, py::arg("type"));
  m.def("mapTypeToXir", proteus::mapTypeToXir, py::arg("type"));
#endif
}
