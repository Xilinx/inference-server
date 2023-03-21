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

list(APPEND tests soft soft_batching)

list(
  APPEND tests_libs
         "fake_observation~$<TARGET_OBJECTS:fake_worker_info_buffers_infinite>~\
            parameters~data_types~batching~memory_pool~buffers~\
            data_types_internal~inference_request~inference_response"
         "fake_observation~$<TARGET_OBJECTS:fake_worker_info_buffers_finite>~\
            data_types~parameters~batching~buffers~memory_pool~\
            data_types_internal~inference_request~inference_response"
)

amdinfer_add_unit_tests("${tests}" "${tests_libs}")
