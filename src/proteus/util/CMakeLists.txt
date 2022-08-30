# Copyright 2021 Xilinx Inc.
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

list(
  APPEND targets
         base64
         compression
         parse_env
         exec
)

list(APPEND UTIL_OBJS)

foreach(target ${targets})
  add_library(${target} OBJECT ${target}.cpp)
  enable_ipo_on_target(${target})
  target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/src)

  list(APPEND UTIL_OBJS $<TARGET_OBJECTS:${target}>)
endforeach()

target_link_libraries(base64 INTERFACE b64)
target_link_libraries(compression INTERFACE z)

add_library(util SHARED ${UTIL_OBJS})
target_link_libraries(util PRIVATE ${targets})
set_target_properties(util PROPERTIES OUTPUT_NAME proteus-util)
