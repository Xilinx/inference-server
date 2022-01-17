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

include(GoogleTest)

list(APPEND targets
    vector_buffer
)

foreach(target ${targets})
    add_executable(${target}_test ${target}.cpp)
    target_include_directories(${target}_test BEFORE PRIVATE ${PROTEUS_TEST_INCLUDE_DIRS})
    target_include_directories(${target}_test AFTER PRIVATE ${PROTEUS_INCLUDE_DIRS})
    target_link_libraries(${target}_test buffer data_types fake_observation ${target} gtest gtest_main)

    gtest_discover_tests(${target}_test)
endforeach()
