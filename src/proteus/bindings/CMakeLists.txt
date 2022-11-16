# Copyright 2022 Xilinx, Inc.
# Copyright 2022 Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if(${AMDINFER_ENABLE_PYTHON_BINDINGS})
  # manylinux builds don't include the Python library
  if(SKBUILD)
    find_package(Python COMPONENTS Interpreter Development.Module)
  else()
    find_package(Python COMPONENTS Interpreter Development)
  endif()
  find_package(pybind11 CONFIG)
  if(${pybind11_FOUND} AND ${Python_FOUND})
    add_subdirectory(python)
  else()
    message(
      STATUS "Pybind11 or Python3 not found, skipping Python binding generation"
    )
  endif()
endif()
