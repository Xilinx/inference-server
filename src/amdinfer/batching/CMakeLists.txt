# Copyright 2021 Xilinx, Inc.
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

list(APPEND files hard soft)
list(APPEND targets)

add_library(batcher OBJECT batcher.cpp)
target_include_directories(batcher PRIVATE ${AMDINFER_INCLUDE_DIRS})
enable_ipo_on_target(batcher)
set(BATCHING_OBJS $<TARGET_OBJECTS:batcher>)

foreach(file ${files})
  set(target ${file}Batcher)
  add_library(${target} OBJECT ${file}.cpp)
  target_include_directories(${target} PRIVATE ${AMDINFER_INCLUDE_DIRS})
  enable_ipo_on_target(${target})

  list(APPEND BATCHING_OBJS $<TARGET_OBJECTS:${target}>)
  list(APPEND targets ${target})
endforeach()

add_library(batching STATIC ${BATCHING_OBJS})
target_link_libraries(batching INTERFACE batcher ${targets})

set(BATCHING_OBJS ${BATCHING_OBJS} PARENT_SCOPE)
