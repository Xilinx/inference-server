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

#include "proteus/bindings/python/clients/clients.hpp"
#include "proteus/bindings/python/core/core.hpp"
#include "proteus/bindings/python/pre_post/pre_post.hpp"
#include "proteus/bindings/python/servers/servers.hpp"
#include "proteus/bindings/python/testing/testing.hpp"
#include "proteus/build_options.hpp"

namespace py = pybind11;

namespace proteus {

PYBIND11_MODULE(_proteus, m) {
  py::module t = m.def_submodule("testing", "testing documentation");
  py::module p = m.def_submodule("pre_post", "pre/post documentation");
  m.doc() = "proteus inference library";

  wrapCore(m);
  wrapClients(m);
  wrapServers(m);
#ifdef PROTEUS_BUILD_TESTING
  wrapTesting(t);
#endif
  wrapPrePost(p);
}

}  // namespace proteus
