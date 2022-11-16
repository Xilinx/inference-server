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

if(${AMDINFER_ENABLE_GRPC})

  list(APPEND tests grpc_internal)
  list(APPEND tests_libs "grpc_internal~lib_grpc~predict_api~data_types")
  amdinfer_add_unit_tests("${tests}" "${tests_libs}")

  amdinfer_get_test_target(grpc_internal_target grpc_internal)
  target_include_directories(
    ${grpc_internal_target}
    PRIVATE $<TARGET_PROPERTY:lib_grpc,INCLUDE_DIRECTORIES>
  )

endif()
