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

list(APPEND TARGETS
    native
    native_internal
)

if(${PROTEUS_ENABLE_HTTP})
    set(TARGETS ${TARGETS} http http_internal)
endif()

if(${PROTEUS_ENABLE_GRPC})
    set(TARGETS ${TARGETS} grpc grpc_internal)
endif()

foreach(target ${TARGETS})
    add_library(${target} OBJECT ${target}.cpp)
    target_include_directories(${target} PRIVATE ${PROTEUS_INCLUDE_DIRS})
    enable_ipo_on_target(${target})

    list(APPEND CLIENT_OBJS $<TARGET_OBJECTS:${target}>)
endforeach()

target_link_libraries(native INTERFACE exec observation)
if(${PROTEUS_ENABLE_AKS})
    target_link_libraries(native INTERFACE aks)
endif()

if(${PROTEUS_ENABLE_HTTP})
    target_link_libraries(http INTERFACE drogon trantor Threads::Threads)
    target_link_libraries(http_internal INTERFACE drogon)
endif()

if(${PROTEUS_ENABLE_GRPC})
    target_include_directories(grpc PRIVATE $<TARGET_PROPERTY:lib_grpc,INCLUDE_DIRECTORIES>)
    target_link_libraries(grpc INTERFACE lib_grpc)
    target_include_directories(grpc_internal PRIVATE $<TARGET_PROPERTY:lib_grpc,INCLUDE_DIRECTORIES>)
    target_link_libraries(grpc_internal INTERFACE lib_grpc)
endif()

add_library(clients INTERFACE)
target_link_libraries(clients INTERFACE
    ${CLIENT_OBJS}
    ${TARGETS}
)

set(CLIENT_OBJS ${CLIENT_OBJS} PARENT_SCOPE)
