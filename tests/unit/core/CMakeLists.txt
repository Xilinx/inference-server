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

add_subdirectory(memory_pool)

list(
  APPEND tests
         inference_request_input
         model_config
         parameter_map
)

list(APPEND tests_libs "inference_request~parameters~inference_response"
            "model_config~tensor~data_types~parameters~util" "parameters"
)

amdinfer_add_unit_tests("${tests}" "${tests_libs}")

amdinfer_get_test_target(target model_config)
target_link_libraries(${target} PRIVATE tomlplusplus::tomlplusplus)
