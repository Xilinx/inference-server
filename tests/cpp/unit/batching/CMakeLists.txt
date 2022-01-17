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
    soft
    soft_batching
)

list(APPEND targets_libs
    "$<TARGET_OBJECTS:fake_worker_info_buffers_infinite>~fake_observation~fake_manager~interface~predict_api~data_types~$<TARGET_OBJECTS:softBatcher>"
    "$<TARGET_OBJECTS:fake_worker_info_buffers_finite>~fake_observation~fake_manager~interface~predict_api~data_types~$<TARGET_OBJECTS:softBatcher>~vector_buffer~buffer"
)

foreach(target lib_str IN ZIP_LISTS targets targets_libs)
    string(REPLACE "~" ";" libs ${lib_str})

    add_executable(${target}_test ${target}.cpp)
    target_include_directories(${target}_test BEFORE PRIVATE ${PROTEUS_TEST_INCLUDE_DIRS})
    target_include_directories(${target}_test AFTER PRIVATE ${PROTEUS_INCLUDE_DIRS})
    target_link_libraries(${target}_test $<TARGET_OBJECTS:batcher> ${libs} gtest gtest_main)

    gtest_discover_tests(${target}_test)
endforeach()
