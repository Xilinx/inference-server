# Copyright (c) 2021 Alex Reinking
# Copyright (c) 2021 Xilinx, Inc.
# Copyright (c) 2022 Advanced Micro Devices, Inc.

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.19)

set(amdinfer_known_comps static shared)
set(amdinfer_comp_static NO)
set(amdinfer_comp_shared NO)
foreach(amdinfer_comp IN LISTS ${CMAKE_FIND_PACKAGE_NAME}_FIND_COMPONENTS)
  if(amdinfer_comp IN_LIST amdinfer_known_comps)
    set(amdinfer_comp_${amdinfer_comp} YES)
  else()
    set(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE
        "amdinfer does not recognize component `${amdinfer_comp}`."
    )
    set(${CMAKE_FIND_PACKAGE_NAME}_FOUND FALSE)
    return()
  endif()
endforeach()

if(amdinfer_comp_static AND amdinfer_comp_shared)
  set(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE
      "amdinfer `static` and `shared` components are mutually exclusive."
  )
  set(${CMAKE_FIND_PACKAGE_NAME}_FOUND FALSE)
  return()
endif()

set(amdinfer_static_targets
    "${CMAKE_CURRENT_LIST_DIR}/amdinfer-static-targets.cmake"
)
set(amdinfer_shared_targets
    "${CMAKE_CURRENT_LIST_DIR}/amdinfer-shared-targets.cmake"
)

macro(amdinfer_load_targets type)
  if(NOT EXISTS "${amdinfer_${type}_targets}")
    set(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE
        "amdinfer `${type}` libraries were requested but not found."
    )
    set(${CMAKE_FIND_PACKAGE_NAME}_FOUND FALSE)
    return()
  endif()
  include("${amdinfer_${type}_targets}")
endmacro()

if(amdinfer_comp_static)
  amdinfer_load_targets(static)
elseif(amdinfer_comp_shared)
  amdinfer_load_targets(shared)
elseif(DEFINED amdinfer_SHARED_LIBS AND amdinfer_SHARED_LIBS)
  amdinfer_load_targets(shared)
elseif(DEFINED amdinfer_SHARED_LIBS AND NOT amdinfer_SHARED_LIBS)
  amdinfer_load_targets(static)
elseif(AMDINFER_BUILD_SHARED)
  if(EXISTS "${amdinfer_shared_targets}")
    amdinfer_load_targets(shared)
  else()
    amdinfer_load_targets(static)
  endif()
else()
  if(EXISTS "${amdinfer_static_targets}")
    amdinfer_load_targets(static)
  else()
    amdinfer_load_targets(shared)
  endif()
endif()
