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

set(base_targets
    base64
    compression
    ctpl
    exec
    filesystem
    parse_env
    read_nth_line
    timer
)
set(derived_targets "")
amdinfer_add_targets(
  targets target_objects "${base_targets}" "${derived_targets}" ""
)

target_link_libraries(base64 INTERFACE b64)
target_link_libraries(compression INTERFACE z)
target_link_libraries(exec INTERFACE Threads::Threads)

add_library(util INTERFACE)
target_link_libraries(util INTERFACE ${targets} ${target_objects})
