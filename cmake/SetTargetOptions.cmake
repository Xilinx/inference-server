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

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT output)

function(enable_ipo_on_target target)
  if(ipo_supported)
    set_property(
      TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION
                                ${AMDINFER_ENABLE_IPO}
    )
  else()
    if(${AMDINFER_ENABLE_IPO})
      message(WARNING "IPO is not supported: ${output}")
    endif()
  endif()
endfunction()

function(set_target_options target)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(
      ${target} PRIVATE "-Wall" "-Wextra" "-Werror" "-Wpedantic"
                        "-pedantic-errors"
    )
  else()
    message(WARNING "Not compiling with gcc. This is not well-tested.")
  endif()

  enable_ipo_on_target(${target})
endfunction()
