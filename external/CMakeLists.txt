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

add_custom_target(extra)

if(${AMDINFER_ENABLE_AKS})
  set(AKS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/aks)

  foreach(DIR graph_zoo kernel_src kernel_zoo cmake-kernels.sh)
    if(NOT EXISTS ${AKS_PATH}/${DIR})
      file(COPY ${AKS_PATH}/reference/${DIR} DESTINATION ${AKS_PATH})
    endif()
  endforeach()

  file(MAKE_DIRECTORY ${AKS_PATH}/libs)

  file(GLOB_RECURSE AKS_KERNELS CONFIGURE_DEPENDS ${AKS_PATH}/kernel_src/*.cpp
       ${AKS_PATH}/kernel_src/*.hpp
  )

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/aks.out
    DEPENDS ${AKS_KERNELS}
    COMMAND bash cmake-kernels.sh --type release >
            ${CMAKE_CURRENT_BINARY_DIR}/aks.out 2>&1
    WORKING_DIRECTORY ${AKS_PATH}
    COMMENT "Building AKS kernels"
    VERBATIM
  )

  add_custom_target(aks-kernels DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/aks.out)

endif()
