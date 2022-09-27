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

add_library(test_main STATIC test_main.cpp)
target_link_libraries(test_main PUBLIC gtest)

list(APPEND files get_asset)
list(APPEND targets)

foreach(file ${files})
  set(target ${file})
  add_library(${target} OBJECT ${file}.cpp)
  target_include_directories(
    ${target} PRIVATE ${PROTEUS_TEST_INCLUDE_DIRS} ${PROTEUS_INCLUDE_DIRS}
  )
  enable_ipo_on_target(${target})

  list(APPEND targets ${target} $<TARGET_OBJECTS:${target}>)
endforeach()

add_library(testing INTERFACE)
target_link_libraries(testing INTERFACE ${targets})
