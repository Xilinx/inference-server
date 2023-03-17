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
 * @brief Implements the Python bindings for the parameters.hpp header
 */

#include "amdinfer/core/parameters.hpp"

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

void wrapParameterMap(py::module_ &m) {
  using amdinfer::ParameterMap;

  py::class_<ParameterMap, std::shared_ptr<ParameterMap>>(m, "ParameterMap")
    .def(py::init<>(), DOCS(ParameterMap))
    .def("put",
         py::overload_cast<const std::string &, bool>(&ParameterMap::put),
         DOCS(ParameterMap, put))
    .def("put",
         py::overload_cast<const std::string &, double>(&ParameterMap::put),
         DOCS(ParameterMap, put, 2))
    .def("put",
         py::overload_cast<const std::string &, int32_t>(&ParameterMap::put),
         DOCS(ParameterMap, put, 3))
    .def("put",
         py::overload_cast<const std::string &, const std::string &>(
           &ParameterMap::put),
         DOCS(ParameterMap, put, 4))
    .def(
      "put",
      py::overload_cast<const std::string &, const char *>(&ParameterMap::put),
      DOCS(ParameterMap, put, 5))
    .def("getBool", &ParameterMap::get<bool>, DOCS(ParameterMap, get))
    .def("getFloat", &ParameterMap::get<double>, DOCS(ParameterMap, get))
    .def("getInt", &ParameterMap::get<int32_t>, DOCS(ParameterMap, get))
    .def("getString", &ParameterMap::get<std::string>, DOCS(ParameterMap, get))
    .def("has", &ParameterMap::has, DOCS(ParameterMap, has), py::arg("key"))
    .def("erase", &ParameterMap::erase, DOCS(ParameterMap, erase))
    .def("empty", &ParameterMap::empty, DOCS(ParameterMap, empty))
    .def("size", &ParameterMap::size, DOCS(ParameterMap, size))
    .def("__len__", &ParameterMap::size)
    .def("__bool__", [](const ParameterMap &self) { return !self.empty(); })
    .def(
      "__iter__",
      [](const ParameterMap &self) {
        return py::make_iterator(self.cbegin(), self.cend());
      },
      KeepAliveReturn())
    .def("__repr__",
         [](const ParameterMap &self) {
           return "ParameterMap(" + std::to_string(self.size()) + ")\n";
         })
    .def("__str__", &amdinfer::toString<ParameterMap>);
}

// refer to cppreference for std::visit
// helper type for the visitor #4
// template <class... Ts>
// struct overloaded : Ts... {
//   using Ts::operator()...;
// };
// // explicit deduction guide (not needed as of C++20)
// template <class... Ts>
// overloaded(Ts...) -> overloaded<Ts...>;

//? Trying to auto-convert ParameterMap <-> dict but it's not working
// namespace pybind11 { namespace detail {
//     template <> struct type_caster<amdinfer::ParameterMap> {
//     public:
//         PYBIND11_TYPE_CASTER(amdinfer::ParameterMap,
//         const_name("ParameterMap"));

//         // Conversion part 1 (Python->C++)
//         bool load(handle src, bool) {
//           /* Try converting into a Python dictionary */
//           auto tmp = py::cast<py::dict>(src);
//           if (!tmp)
//             return false;

//           for(const auto& [key, param] : tmp){
//             const auto& key_str = PyUnicode_AsUTF8(key.ptr());
//             bool foo;
//             try{
//               PyArg_ParseTuple(param.ptr(), "p", &foo);
//             } catch(...){
//               int foo2;
//               try{

//                 PyArg_ParseTuple(param.ptr(), "i", &foo);
//               } catch(...){
//                 double foo3;
//                 try{
//                   PyArg_ParseTuple(param.ptr(), "d", &foo);
//                 } catch(...){
//                   const char* foo4;
//                   PyArg_ParseTuple(param.ptr(), "s", &foo);
//                   value.put(key_str, foo4);
//                   continue;
//                 }
//                 value.put(key_str, foo3);
//                 continue;
//               }
//               value.put(key_str, foo2);
//               continue;
//             }
//             value.put(key_str, foo);
//             continue;
//           }

//           Py_DECREF(tmp.ptr());
//           // Ensure return code was OK
//           return !PyErr_Occurred();
//         }

//         /**
//          * Conversion part 2 (C++ -> Python). Ignoring policy and handle per
//          * pybind11 suggestion
//          */
//         static handle cast(amdinfer::ParameterMap src,
//         return_value_policy, handle) {
//           py::dict tmp;
//           for (const auto& pair : src) {
//             auto& key = pair.first;
//             const auto& param = pair.second;
//             std::visit(
//               overloaded{[&](bool arg) { tmp[pybind11::cast(key)] = arg; },
//                         [&](double arg) { tmp[pybind11::cast(key)] = arg; },
//                         [&](int32_t arg) { tmp[pybind11::cast(key)] = arg; },
//                         [&](const std::string& arg) {
//                         tmp[pybind11::cast(key)] = arg; }},
//               param);
//           }
//           return tmp;
//         }
//     };
// }} // namespace pybind11::detail

}  // namespace amdinfer
