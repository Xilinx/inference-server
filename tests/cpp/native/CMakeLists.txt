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

if(${PROTEUS_ENABLE_VITIS})
    add_executable(xmodel xmodel.cpp)
    target_link_libraries(xmodel PRIVATE
        proteus
        opencv_core opencv_imgcodecs
        xir vart-runner vart-dpu-controller vart-dpu-runner rt-engine
    )
    target_include_directories(xmodel PRIVATE ${PROTEUS_PUBLIC_INCLUDE_DIRS})
    set(targets ${targets} xmodel)
endif()

if(${PROTEUS_ENABLE_AKS})
    add_executable(facedetect facedetect.cpp)
    target_link_libraries(facedetect PRIVATE
        proteus
        opencv_core opencv_imgcodecs
    )
    target_include_directories(facedetect PRIVATE ${PROTEUS_PUBLIC_INCLUDE_DIRS})
    set(targets ${targets} facedetect)
endif()

set(TARGETS ${targets} PARENT_SCOPE)
