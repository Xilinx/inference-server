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

if(${PROTEUS_ENABLE_VITIS})
    list(APPEND apps custom_processing_native)
    if(${PROTEUS_ENABLE_GRPC})
      list(APPEND apps custom_processing_grpc)
    endif()
    foreach(app ${apps})
        add_executable(${app} custom_processing.cpp)

        target_link_libraries(${app} PRIVATE
            proteus
            opencv_imgcodecs opencv_imgproc opencv_core
        )
        target_include_directories(${app} PRIVATE ${PROTEUS_PUBLIC_INCLUDE_DIRS})
    endforeach()
    if(${PROTEUS_ENABLE_GRPC})
      target_compile_definitions(custom_processing_grpc PRIVATE ENABLE_GRPC)
    endif()
endif()

if(${PROTEUS_ENABLE_TFZENDNN})
  add_executable(tf_zendnn_client tf_zendnn_client.cpp)

  target_link_libraries(tf_zendnn_client PRIVATE
    proteus
    opencv_imgcodecs opencv_imgproc opencv_core
  )
  target_include_directories(tf_zendnn_client PRIVATE ${PROTEUS_PUBLIC_INCLUDE_DIRS})
endif()

if(${PROTEUS_ENABLE_PTZENDNN})
  add_executable(pt_zendnn_client pt_zendnn_client.cpp)

  target_link_libraries(pt_zendnn_client PRIVATE
    proteus
    opencv_imgcodecs opencv_imgproc opencv_core
  )
  target_include_directories(pt_zendnn_client PRIVATE ${PROTEUS_PUBLIC_INCLUDE_DIRS})
endif()
