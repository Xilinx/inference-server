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
 * @brief Implements the Python bindings for the package
 */

#include <pybind11/pybind11.h>

#include "proteus/bindings/python/client_operators/client_operators.hpp"
#include "proteus/bindings/python/clients/client.hpp"
#include "proteus/bindings/python/core/data_types.hpp"
#include "proteus/bindings/python/core/predict_api.hpp"
#include "proteus/bindings/python/servers/server.hpp"
#include "proteus/bindings/python/testing/testing.hpp"
#include "proteus/bindings/python/util/util.hpp"
#include "proteus/core/exceptions.hpp"

namespace py = pybind11;

namespace proteus {

PYBIND11_MODULE(_proteus, m) {
  py::module n = m.def_submodule("predict_api", "predict_api documentation");
  py::module f =
    m.def_submodule("client_operators", "client operators documentation");
  py::module c = m.def_submodule("clients", "client documentation");
  py::module s = m.def_submodule("servers", "server documentation");
  py::module t = m.def_submodule("testing", "testing documentation");
  py::module u = m.def_submodule("util", "util documentation");
  m.doc() = "proteus inference library";

  py::register_exception<runtime_error>(m, "RuntimeError");

  wrapRequestParameters(m);
  wrapDataType(m);
  wrapTypeMaps(m);
  wrapPredictApi(n);
  wrapClient(c);
  wrapHttpClient(c);
  wrapWebSocketClient(c);
  wrapServer(s);
  wrapInferAsync(f);
  wrapTesting(t);
  wrapUtil(u);
}

}  // namespace proteus
