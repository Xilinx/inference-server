# Copyright 2023 Advanced Micro Devices, Inc.
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

include(SetTargetOptions)

function(amdinfer_add_object_library target file)
  add_library(${target} OBJECT ${file}.cpp)
  target_include_directories(${target} PRIVATE ${AMDINFER_INCLUDE_DIRS})
  set_target_options(${target})
endfunction()

function(amdinfer_add_protobuf_target target file build_grpc)
  add_library(${target} STATIC ${file})
  disable_target_linting(${target})
  target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  set_target_options(${target})
  protobuf_generate(TARGET ${target} LANGUAGE cpp)
  target_include_directories(
    ${target} SYSTEM
    PRIVATE
      $<TARGET_PROPERTY:protobuf::libprotobuf,INTERFACE_INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${target} INTERFACE protobuf::libprotobuf)
  if(${build_grpc})
    protobuf_generate(
      TARGET ${target}
      LANGUAGE grpc
      GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
      PLUGIN "protoc-gen-grpc=$<TARGET_PROPERTY:gRPC::grpc_cpp_plugin,LOCATION>"
    )
    target_link_libraries(${target} INTERFACE gRPC::grpc++)
  endif()
endfunction()

function(amdinfer_add_targets targets target_objects base_targets
         derived_targets suffix
)
  set(new_targets ${base_targets})

  foreach(target ${base_targets})
    amdinfer_add_object_library(${target} ${target})
  endforeach()

  foreach(file ${derived_targets})
    set(target ${file}${suffix})
    amdinfer_add_object_library(${target} ${file})
    list(APPEND new_targets ${target})
  endforeach()

  set(new_target_objects ${new_targets})
  list(
    TRANSFORM new_target_objects
              REPLACE
              "^(.+)$"
              "$<TARGET_OBJECTS:\\1>"
  )

  set(${targets} ${new_targets} PARENT_SCOPE)
  set(${target_objects} ${new_target_objects} PARENT_SCOPE)
endfunction()
