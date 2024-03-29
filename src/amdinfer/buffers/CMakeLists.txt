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

set(base_targets buffer)
set(derived_targets cpu vector)
if(${AMDINFER_ENABLE_VITIS})
  list(APPEND derived_targets vart_tensor)
endif()
amdinfer_add_targets(
  targets target_objects "${base_targets}" "${derived_targets}" _buffer
)

if(${AMDINFER_ENABLE_VITIS})
  target_link_libraries(vart_tensor_buffer INTERFACE vart::runner)
endif()

add_library(buffers INTERFACE)
target_link_libraries(buffers INTERFACE ${targets} ${target_objects})
