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
 * @brief Defines the objects for Python bindings
 */

#ifndef GUARD_PROTEUS_BINDINGS_PYTHON_PREDICT_API
#define GUARD_PROTEUS_BINDINGS_PYTHON_PREDICT_API

namespace pybind11 {
class module_;
}

void wrapRequestParameters(pybind11::module_ &);
void wrapPredictApi(pybind11::module_ &);

#endif  // GUARD_PROTEUS_BINDINGS_PYTHON_PREDICT_API
