# Copyright 2022 Advanced Micro Devices Inc.
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

list(APPEND targets resnet50)

foreach(target ${targets})
  set(test testPerfModels${target})

  add_executable(${test} ${target}.cpp)
  target_include_directories(
    ${test} BEFORE PRIVATE ${PROTEUS_TEST_INCLUDE_DIRS}
  )
  target_include_directories(${test} AFTER PRIVATE ${PROTEUS_INCLUDE_DIRS})
  target_link_libraries(
    ${test}
    proteus
    gtest
    gtest_main
    opencv_imgcodecs
    opencv_imgproc
    opencv_core
  )

  gtest_discover_tests(${test})
endforeach()
