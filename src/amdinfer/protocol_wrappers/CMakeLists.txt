# Copyright 2023 Advanced Micro Devices, Inc.
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

list(APPEND targets protocol_wrapper)

foreach(target ${targets})
  add_library(${target} OBJECT ${target}.cpp)
  target_include_directories(${target} PRIVATE ${AMDINFER_INCLUDE_DIRS})
  set_target_options(${target})

  list(APPEND PROTOCOL_WRAPPER_OBJS $<TARGET_OBJECTS:${target}>)
endforeach()

add_library(protocol_wrappers STATIC ${PROTOCOL_WRAPPER_OBJS})
target_link_libraries(protocol_wrappers INTERFACE ${targets})
set_target_options(protocol_wrappers)

set(PROTOCOL_WRAPPER_OBJS ${PROTOCOL_WRAPPER_OBJS} PARENT_SCOPE)
