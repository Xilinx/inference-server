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

set(base_targets client native)
set(derived_targets "")
if(${AMDINFER_ENABLE_HTTP})
  list(
    APPEND base_targets
           http
           http_internal
           websocket
  )
endif()
if(${AMDINFER_ENABLE_GRPC})
  list(APPEND base_targets grpc grpc_internal)
endif()
amdinfer_add_targets(
  targets target_objects "${base_targets}" "${derived_targets}" _client
)

if(${AMDINFER_ENABLE_HTTP})
  target_link_libraries(http_internal PUBLIC Drogon::Drogon)
  target_link_libraries(http PRIVATE Drogon::Drogon)
  target_link_libraries(websocket PRIVATE Drogon::Drogon)
endif()

if(${AMDINFER_ENABLE_GRPC})
  target_include_directories(
    grpc PRIVATE $<TARGET_PROPERTY:lib_grpc,INCLUDE_DIRECTORIES>
  )
  target_include_directories(
    grpc_internal PRIVATE $<TARGET_PROPERTY:lib_grpc,INCLUDE_DIRECTORIES>
  )

  target_link_libraries(grpc INTERFACE lib_grpc)
endif()

add_library(clients INTERFACE)
target_link_libraries(clients INTERFACE ${targets} ${target_objects})
