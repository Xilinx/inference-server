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

#include "proteus/bindings/python/predict_api.hpp"

namespace py = pybind11;

namespace proteus {

PYBIND11_MODULE(proteus, m) {
  // py::module n = m.def_submodule("predict_api", "predict_api documentation");
  m.doc() = "pybind11 example plugin";
  initRequestParameters(m);
}

}  // namespace proteus
