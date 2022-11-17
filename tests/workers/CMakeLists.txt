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

list(APPEND tests echo)

if(${AMDINFER_ENABLE_AKS})
  set(tests ${tests} facedetect)
endif()

amdinfer_add_system_tests("${tests}")

if(${AMDINFER_ENABLE_AKS})
  amdinfer_get_test_target(facedetect_target facedetect)
  target_link_libraries(
    ${facedetect_target} PRIVATE opencv_core opencv_imgcodecs
  )
endif()

foreach(test ${tests})
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${test}.cpp)
    amdinfer_get_test_target(target ${test} "exec")
    amdinfer_get_test_target(test_target ${test})
    add_executable(${target} ${test}.cpp)
    target_link_libraries(
      ${target} PRIVATE $<TARGET_PROPERTY:${test_target},LINK_LIBRARIES>
    )
    target_include_directories(
      ${target} PRIVATE ${AMDINFER_PUBLIC_INCLUDE_DIRS}
    )
  endif()
endforeach()

if(${AMDINFER_ENABLE_VITIS})
  add_subdirectory(xmodel)
endif()
