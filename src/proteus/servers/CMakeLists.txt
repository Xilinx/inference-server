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

list(APPEND targets)

if(${PROTEUS_ENABLE_REST})
    set(targets ${targets} http_server websocket_server)
endif()

if(${PROTEUS_ENABLE_GRPC})
    set(targets ${targets} grpc_server)
endif()

foreach(target ${targets})
    add_library(${target} OBJECT ${target}.cpp)
    target_include_directories(${target} PRIVATE ${PROTEUS_INCLUDE_DIRS})
    enable_ipo_on_target(${target})

    list(APPEND SERVER_OBJS $<TARGET_OBJECTS:${target}>)
endforeach()

if(${PROTEUS_ENABLE_REST})
    target_link_libraries(http_server INTERFACE
        clients core compression observation
        drogon trantor jsoncpp
    )
    target_link_libraries(websocket_server INTERFACE observation drogon trantor)
endif()

if(${PROTEUS_ENABLE_GRPC})
    target_include_directories(grpc_server PRIVATE $<TARGET_PROPERTY:lib_grpc,INCLUDE_DIRECTORIES>)
    target_link_libraries(grpc_server INTERFACE lib_grpc)
endif()

add_library(servers INTERFACE)
target_link_libraries(servers INTERFACE
    ${SERVER_OBJS}
    ${targets}
)

set(SERVER_OBJS ${SERVER_OBJS} PARENT_SCOPE)
