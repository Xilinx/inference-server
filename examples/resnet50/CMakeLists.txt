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

list(APPEND examples)

if(${PROTEUS_ENABLE_VITIS})
  list(APPEND examples vitis)
endif()

if(${PROTEUS_ENABLE_PTZENDNN})
  list(APPEND examples ptzendnn)
endif()

if(${PROTEUS_ENABLE_TFZENDNN})
  list(APPEND examples tfzendnn)
endif()

if(${PROTEUS_ENABLE_MIGRAPHX})
  list(APPEND examples migraphx)
endif()

foreach(example ${examples})
  amdinfer_add_example(${example})
  amdinfer_get_example_target(target ${example})
  target_link_libraries(
    ${target} PRIVATE opencv_imgcodecs opencv_imgproc opencv_core
  )
endforeach()

# if(${PROTEUS_ENABLE_TFZENDNN})
#   add_executable(tf_zendnn_client tf_zendnn_client.cpp)

#   target_link_libraries(
#     tf_zendnn_client PRIVATE amdinfer util opencv_imgcodecs opencv_imgproc
#                              opencv_core
#   )
#   target_include_directories(tf_zendnn_client PRIVATE ${PROTEUS_INCLUDE_DIRS})
# endif()

# if(${PROTEUS_ENABLE_PTZENDNN})
#   add_executable(pt_zendnn_client pt_zendnn_client.cpp)

#   target_link_libraries(
#     pt_zendnn_client PRIVATE amdinfer util opencv_imgcodecs opencv_imgproc
#                              opencv_core
#   )
#   target_include_directories(pt_zendnn_client PRIVATE ${PROTEUS_INCLUDE_DIRS})
# endif()

# if(${PROTEUS_ENABLE_MIGRAPHX})
#   add_executable(migraphx_client migraphx_client.cpp)

#   target_link_libraries(
#     migraphx_client PRIVATE amdinfer util opencv_imgcodecs opencv_imgproc
#                             opencv_core migraphx::c
#   )
#   target_include_directories(migraphx_client PRIVATE ${PROTEUS_INCLUDE_DIRS})
# endif()
