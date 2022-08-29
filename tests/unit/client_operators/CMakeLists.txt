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

include(GoogleTest)

list(APPEND targets infer_async)

foreach(target ${targets})
  add_executable(test_${target} test_${target}.cpp)
  target_include_directories(
    test_${target} BEFORE PRIVATE ${PROTEUS_TEST_INCLUDE_DIRS}
  )
  target_include_directories(
    test_${target} AFTER PRIVATE ${PROTEUS_INCLUDE_DIRS}
  )
  target_link_libraries(test_${target} proteus gtest gtest_main)

  gtest_discover_tests(test_${target})
endforeach()
