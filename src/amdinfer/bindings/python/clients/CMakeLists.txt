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

list(APPEND targets client native)

if(${AMDINFER_ENABLE_HTTP})
  list(APPEND targets http websocket)
endif()

if(${AMDINFER_ENABLE_GRPC})
  list(APPEND targets grpc)
endif()

foreach(target ${targets})
  add_library(${target}_py OBJECT ${target}.cpp)
  target_include_directories(
    ${target}_py PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/..
                         ${AMDINFER_INCLUDE_DIRS} ${Python_INCLUDE_DIRS}
  )
  target_link_libraries(${target}_py PRIVATE ${Python_LIBRARIES} amdinfer)
  add_dependencies(${target}_py pybind11Mkdoc)

  set(LINK_TARGETS ${LINK_TARGETS} ${target}_py)
endforeach()

set(CLIENT_OBJS ${LINK_TARGETS} PARENT_SCOPE)
