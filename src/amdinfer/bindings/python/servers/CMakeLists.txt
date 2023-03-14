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

set(derived_targets server)

amdinfer_add_targets(targets target_objects "" "${derived_targets}" _py)

foreach(target ${derived_targets})
  target_include_directories(
    ${target}_py PRIVATE ${AMDINFER_PYTHON_INCLUDE_DIRS}
  )
endforeach()

add_library(servers_py INTERFACE)
target_link_libraries(servers_py INTERFACE ${targets} ${target_objects})
add_dependencies(servers_py pybind11Mkdoc)
