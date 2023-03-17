# Copyright 2022 Xilinx, Inc.
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

list(
  APPEND targets
         fake_inference_request
         worker_info_buffers_infinite
         worker_info_buffers_finite
         # data_types
)

foreach(target ${targets})
  add_library(fake_${target} OBJECT ${target}.cpp)
  target_include_directories(
    fake_${target} BEFORE PRIVATE ${AMDINFER_TEST_INCLUDE_DIRS}
  )
  target_include_directories(
    fake_${target} AFTER PRIVATE ${AMDINFER_INCLUDE_DIRS}
  )
  set_target_options(fake_${target})

  list(APPEND CORE_OBJS $<TARGET_OBJECTS:fake_${target}>)
endforeach()

add_library(fake_core INTERFACE)
target_link_libraries(fake_core INTERFACE ${CORE_OBJS} ${targets})
