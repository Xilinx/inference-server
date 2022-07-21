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
         predict_api
         manager
         worker_info
         data_types
         interface
         model_repository
         api
)

foreach(target ${targets})
  add_library(${target} OBJECT ${target}.cpp)
  target_include_directories(${target} PRIVATE ${PROTEUS_INCLUDE_DIRS})
  enable_ipo_on_target(${target})

  list(APPEND CORE_OBJS $<TARGET_OBJECTS:${target}>)
endforeach()

target_link_libraries(manager INTERFACE Threads::Threads)

target_link_libraries(predict_api INTERFACE jsoncpp)
target_link_libraries(worker_info INTERFACE ${CMAKE_DL_LIBS})
if(${PROTEUS_ENABLE_VITIS})
  target_link_libraries(data_types INTERFACE xir)
  target_link_libraries(api INTERFACE sockpp)
endif()

if(${PROTEUS_ENABLE_GRPC})
  add_library(lib_grpc STATIC predict_api.proto)
  disable_target_linting(lib_grpc)
  target_include_directories(lib_grpc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(lib_grpc INTERFACE gRPC::grpc++)

  get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
  protobuf_generate(TARGET lib_grpc LANGUAGE cpp)
  protobuf_generate(
    TARGET lib_grpc
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}"
  )
endif()

add_library(lib_config STATIC model_config.proto)
target_include_directories(lib_config PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(lib_config INTERFACE protobuf::libprotobuf)
protobuf_generate(TARGET lib_config LANGUAGE cpp)
disable_target_linting(lib_config)

target_include_directories(
  model_repository PRIVATE $<TARGET_PROPERTY:lib_config,INCLUDE_DIRECTORIES>
)
target_link_libraries(model_repository INTERFACE lib_config efsw)

add_library(core STATIC ${CORE_OBJS})
target_link_libraries(core INTERFACE ${targets})

set(CORE_OBJS ${CORE_OBJS} PARENT_SCOPE)
