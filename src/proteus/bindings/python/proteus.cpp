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
 * @brief Implements the Python bindings for the package
 */

#include <pybind11/pybind11.h>

#include "proteus/bindings/python/clients/client.hpp"
#include "proteus/bindings/python/core/data_types.hpp"
#include "proteus/bindings/python/core/predict_api.hpp"
#include "proteus/clients/native.hpp"
#include "proteus/core/exceptions.hpp"

namespace py = pybind11;

namespace proteus {

PYBIND11_MODULE(_proteus, m) {
  py::module n = m.def_submodule("predict_api", "predict_api documentation");
  py::module c = m.def_submodule("clients", "client documentation");
  m.doc() = "proteus inference library";

  m.def("initialize", &initialize);
  m.def("initializeLogging", &initializeLogging);
  m.def("terminate", &terminate);

  py::register_exception<runtime_error>(m, "RuntimeError");

  wrapRequestParameters(m);
  wrapDataType(m);
  wrapTypeMaps(m);
  wrapPredictApi(n);
  wrapClient(c);
  wrapHttpClient(c);
  wrapWebSocketClient(c);
}

}  // namespace proteus
