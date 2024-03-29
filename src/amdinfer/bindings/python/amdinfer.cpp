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

#include "amdinfer/bindings/python/clients/clients.hpp"
#include "amdinfer/bindings/python/core/core.hpp"
#include "amdinfer/bindings/python/pre_post/pre_post.hpp"
#include "amdinfer/bindings/python/servers/servers.hpp"
#include "amdinfer/bindings/python/testing/testing.hpp"
#include "amdinfer/build_options.hpp"

namespace py = pybind11;

namespace amdinfer {

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
PYBIND11_MODULE(_amdinfer, m) {
  py::module t = m.def_submodule("testing", "testing documentation");
  py::module p = m.def_submodule("pre_post", "pre/post documentation");
  m.doc() = "amdinfer inference library";

  wrapCore(m);
  // server needs to be bound before NativeClient
  wrapServers(m);
  wrapClients(m);
#ifdef AMDINFER_BUILD_TESTING
  wrapTesting(t);
#endif
  wrapPrePost(p);
}

}  // namespace amdinfer
