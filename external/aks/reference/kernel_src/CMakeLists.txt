# Copyright 2019 Xilinx Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(kernels
    AddKernel
    ClassificationAccuracy
    ClassificationImreadPre
    ClassificationImreadResizeCCrop
    ClassificationPostProc
    ClassificationPreProc
    DetectionImreadPre
    DetectionPreproc
    DpuRunner
    FaceDetectImreadPreprocess
    FaceDetectPostprocess
    FaceDetectPreprocess
    ImageRead
    MeanSubtract
    SaveBoxesDarknetFacedetect
    SaveBoxesDarknetFormat
)

function(amdinfer_get_kernel_target target filename worker)
  # convert name to file name: separate by capital letters, add underscores and
  # lowercase
  string(REGEX MATCHALL "[A-Z][a-z0-9]*" worker_list "${worker}")
  string(REPLACE ";" "_" worker_list_str "${worker_list}")
  string(TOLOWER ${worker_list_str} file_name)

  set(${target} Aks${worker} PARENT_SCOPE)
  set(${filename} ${file_name} PARENT_SCOPE)
endfunction()

foreach(kernel ${kernels})
  amdinfer_get_kernel_target(target filename ${kernel})
  add_library(${target} SHARED ${filename}.cpp)

  target_link_libraries(
    ${target} PRIVATE aks::aks opencv_core opencv_imgproc opencv_imgcodecs
  )

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMENT "Linking ${target} to $AKS_ROOT/libs at ${AKS_PATH}/libs"
    COMMAND cp -fs ${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:${target}>
            ${AKS_PATH}/libs
    COMMAND symlinks -c ${AKS_PATH}/libs > /dev/null
  )
endforeach()

amdinfer_get_kernel_target(target filename YoloPostproc)
add_library(${target} SHARED ${filename}.cpp yolo.cpp)

target_link_libraries(${target} PRIVATE aks::aks)

add_custom_command(
  TARGET ${target}
  POST_BUILD
  COMMENT "Linking ${target} to $AKS_ROOT/libs at ${AKS_PATH}/libs"
  COMMAND cp -fs ${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:${target}>
          ${AKS_PATH}/libs
  COMMAND symlinks -c ${AKS_PATH}/libs > /dev/null
)
