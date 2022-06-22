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

list(APPEND targets
    test_model_infer
    test_model_list
    test_model_ready
    test_server_live
    test_server_ready
)

if(${PROTEUS_ENABLE_TFZENDNN})
    list(APPEND targets test_model_load)
endif()

foreach(target ${targets})
    add_executable(${target} ${target}.cpp)

    target_include_directories(${target} PRIVATE ${PROTEUS_PUBLIC_INCLUDE_DIRS} ${PROTEUS_TEST_INCLUDE_DIRS})
    target_link_libraries(${target} PRIVATE proteus gtest gtest_main)

    gtest_discover_tests(${target})
endforeach()
