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

list(APPEND targets
  predict_api
  fake_predict_api
  manager
  worker_info
  data_types
  interface
)

foreach(target ${targets})
  add_library(${target} OBJECT ${target}.cpp)
  target_include_directories(${target} PRIVATE ${PROTEUS_INCLUDE_DIRS})
  enable_ipo_on_target(${target})

  list(APPEND CORE_OBJS $<TARGET_OBJECTS:${target}>)
endforeach()

target_link_libraries(predict_api INTERFACE jsoncpp)
target_link_libraries(worker_info INTERFACE ${CMAKE_DL_LIBS})
if(${PROTEUS_ENABLE_VITIS})
  target_link_libraries(data_types INTERFACE xir)
endif()

add_library(core INTERFACE)
target_link_libraries(core INTERFACE
  ${CORE_OBJS}
  ${targets}
)

set(CORE_OBJS ${CORE_OBJS} PARENT_SCOPE)
