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
    list(APPEND apps
        custom_processing_native
        custom_processing_grpc
    )
    foreach(app ${apps})
        add_executable(${app} custom_processing.cpp)

        target_link_libraries(${app} PRIVATE
            proteus
            opencv_imgcodecs opencv_imgproc opencv_core
        )
        target_include_directories(${app} PRIVATE ${PROTEUS_PUBLIC_INCLUDE_DIRS})
    endforeach()
    target_compile_definitions(custom_processing_grpc PRIVATE ENABLE_GRPC)
endif()
