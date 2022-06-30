# Copyright 2022 Xilinx Inc.
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

list(APPEND TARGETS http websocket)

list(APPEND LINK_TARGETS client_py)

add_library(client_py OBJECT client.cpp)
target_include_directories(
  client_py PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/.. ${PROTEUS_INCLUDE_DIRS}
                    ${Python_INCLUDE_DIRS}
)
target_link_libraries(client_py PRIVATE ${Python_LIBRARIES})
add_dependencies(client_py pybind11Mkdoc)

foreach(target ${TARGETS})
  add_library(${target}_py OBJECT ${target}.cpp)
  target_include_directories(
    ${target}_py PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/.. ${PROTEUS_INCLUDE_DIRS}
                         ${Python_INCLUDE_DIRS}
  )
  target_link_libraries(
    ${target}_py PRIVATE ${Python_LIBRARIES} ${target}
                         $<TARGET_OBJECTS:${target}>
  )
  add_dependencies(${target}_py pybind11Mkdoc)

  set(LINK_TARGETS ${LINK_TARGETS} ${target}_py)
endforeach()

# target_link_libraries(data_types_py PRIVATE
# /usr/lib/python3/dist-packages/xir.cpython-36m-x86_64-linux-gnu.so)

set(CLIENT_OBJS ${LINK_TARGETS} PARENT_SCOPE)
