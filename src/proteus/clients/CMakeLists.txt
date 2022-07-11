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

list(
  APPEND targets
         client
         native
         native_internal
)

if(${PROTEUS_ENABLE_HTTP})
  set(targets ${targets} http http_internal websocket)
endif()

if(${PROTEUS_ENABLE_GRPC})
  set(targets ${targets} grpc grpc_internal)
endif()

foreach(target ${targets})
  add_library(${target} OBJECT ${target}.cpp)
  target_include_directories(${target} PRIVATE ${PROTEUS_INCLUDE_DIRS})
  enable_ipo_on_target(${target})

  list(APPEND CLIENT_OBJS $<TARGET_OBJECTS:${target}>)
endforeach()

target_link_libraries(native INTERFACE batching)

if(${PROTEUS_ENABLE_HTTP})
  target_link_libraries(http_internal INTERFACE helpers)
  target_link_libraries(websocket INTERFACE drogon trantor Threads::Threads)
endif()

if(${PROTEUS_ENABLE_GRPC})
  target_include_directories(
    grpc PRIVATE $<TARGET_PROPERTY:lib_grpc,INCLUDE_DIRECTORIES>
  )
  target_include_directories(
    grpc_internal PRIVATE $<TARGET_PROPERTY:lib_grpc,INCLUDE_DIRECTORIES>
  )
endif()

add_library(clients STATIC ${CLIENT_OBJS})
target_link_libraries(clients INTERFACE ${targets})

set(CLIENT_OBJS ${CLIENT_OBJS} PARENT_SCOPE)
