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
)

list(APPEND targets_libs
    "$<TARGET_OBJECTS:fake_worker_info_buffers_finite>~fake_observation~fake_manager~interface~lib_predict_api~data_types~$<TARGET_OBJECTS:softBatcher>~vector_buffer~buffer~$<TARGET_OBJECTS:native_internal>"
)

foreach(target lib_str IN ZIP_LISTS targets targets_libs)
    string(REPLACE "~" ";" libs ${lib_str})

    add_executable(${target}_perf ${target}.cpp)
    target_include_directories(${target}_perf BEFORE PRIVATE ${PROTEUS_TEST_INCLUDE_DIRS})
    target_include_directories(${target}_perf AFTER PRIVATE ${PROTEUS_INCLUDE_DIRS})
    target_link_libraries(${target}_perf $<TARGET_OBJECTS:batcher> ${libs} gtest gtest_main)

    gtest_discover_tests(${target}_perf)
endforeach()
