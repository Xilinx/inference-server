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

set(base_targets server)
if(${AMDINFER_ENABLE_HTTP})
  list(APPEND base_targets http_server websocket_server)
endif()
if(${AMDINFER_ENABLE_GRPC})
  list(APPEND base_targets grpc_server)
endif()
set(derived_targets "")
amdinfer_add_targets(
  targets target_objects "${base_targets}" "${derived_targets}" ""
)

if(${AMDINFER_ENABLE_AKS})
  target_link_libraries(server INTERFACE aks::aks)
endif()

if(${AMDINFER_ENABLE_HTTP})
  target_link_libraries(
    http_server PUBLIC Drogon::Drogon INTERFACE $<TARGET_OBJECTS:http_internal>
  )
  target_link_libraries(websocket_server PUBLIC Drogon::Drogon)
  target_link_libraries(server PUBLIC http_server)
endif()

if(${AMDINFER_ENABLE_GRPC})
  target_include_directories(
    grpc_server SYSTEM PRIVATE $<TARGET_PROPERTY:lib_grpc,INCLUDE_DIRECTORIES>
  )
  target_link_libraries(
    grpc_server INTERFACE lib_grpc $<TARGET_OBJECTS:grpc_internal>
  )
  add_dependencies(grpc_server lib_grpc)
endif()

add_library(servers INTERFACE)
target_link_libraries(servers INTERFACE ${targets} ${target_objects})
