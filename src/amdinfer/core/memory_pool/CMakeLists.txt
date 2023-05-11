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

set(base_targets cpu_allocator pool)
if(${AMDINFER_ENABLE_VITIS})
  list(APPEND base_targets vart_tensor_allocator)
endif()
set(derived_targets "")
amdinfer_add_targets(
  targets target_objects "${base_targets}" "${derived_targets}" ""
)

target_link_libraries(pool INTERFACE $<TARGET_OBJECTS:buffer>)
target_link_libraries(cpu_allocator INTERFACE $<TARGET_OBJECTS:cpu_buffer>)

if(${AMDINFER_ENABLE_VITIS})
  target_link_libraries(
    vart_tensor_allocator INTERFACE vart::runner
                                    $<TARGET_OBJECTS:vart_tensor_buffer>
  )
endif()

add_library(memory_pool INTERFACE)
target_link_libraries(memory_pool INTERFACE ${targets} ${target_objects})
