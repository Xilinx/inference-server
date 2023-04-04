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

add_subdirectory(memory_pool)

set(base_targets
    inference_request
    inference_response
    inference_tensor
    model_config
    tensor
    model_metadata
    endpoints
    worker_info
    data_types
    data_types_internal
    model_repository
    parameters
    shared_state
)
set(derived_targets "")
amdinfer_add_targets(
  targets target_objects "${base_targets}" "${derived_targets}" _core
)

amdinfer_add_protobuf_target(lib_config model_config.proto OFF)

target_include_directories(
  model_repository PRIVATE $<TARGET_PROPERTY:lib_config,INCLUDE_DIRECTORIES>
)
target_link_libraries(model_repository INTERFACE efsw lib_config)
add_dependencies(model_repository lib_config)

target_include_directories(
  model_config PRIVATE $<TARGET_PROPERTY:lib_config,INCLUDE_DIRECTORIES>
)
target_link_libraries(
  model_config PRIVATE tomlplusplus::tomlplusplus INTERFACE lib_config
)

target_link_libraries(inference_tensor INTERFACE $<TARGET_OBJECTS:tensor>)
target_link_libraries(
  inference_request INTERFACE inference_tensor
                              $<TARGET_OBJECTS:inference_tensor>
)
target_link_libraries(
  inference_response INTERFACE inference_tensor
                               $<TARGET_OBJECTS:inference_tensor>
)

target_link_libraries(shared_state INTERFACE Jsoncpp_lib)

if(${AMDINFER_ENABLE_VITIS})
  target_link_libraries(data_types_internal INTERFACE xir)
  target_link_libraries(shared_state INTERFACE sockpp)
endif()

if(${AMDINFER_ENABLE_GRPC})
  amdinfer_add_protobuf_target(lib_grpc inference.proto ON)
endif()

add_library(core INTERFACE)
target_link_libraries(core INTERFACE ${targets} ${target_objects} memory_pool)
