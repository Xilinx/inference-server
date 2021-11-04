# Copyright (c) 2021 Alex Reinking
# Copyright (c) 2021 Xilinx Inc.

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

set(proteus_known_comps static shared)
set(proteus_comp_static NO)
set(proteus_comp_shared NO)
foreach (proteus_comp IN LISTS ${CMAKE_FIND_PACKAGE_NAME}_FIND_COMPONENTS)
    if (proteus_comp IN_LIST proteus_known_comps)
        set(proteus_comp_${proteus_comp} YES)
    else ()
        set(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE
            "proteus does not recognize component `${proteus_comp}`.")
        set(${CMAKE_FIND_PACKAGE_NAME}_FOUND FALSE)
        return()
    endif ()
endforeach ()

if (proteus_comp_static AND proteus_comp_shared)
    set(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE
        "proteus `static` and `shared` components are mutually exclusive.")
    set(${CMAKE_FIND_PACKAGE_NAME}_FOUND FALSE)
    return()
endif ()

set(proteus_static_targets "${CMAKE_CURRENT_LIST_DIR}/proteus-static-targets.cmake")
set(proteus_shared_targets "${CMAKE_CURRENT_LIST_DIR}/proteus-shared-targets.cmake")

macro(proteus_load_targets type)
    if (NOT EXISTS "${proteus_${type}_targets}")
        set(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE
            "proteus `${type}` libraries were requested but not found.")
        set(${CMAKE_FIND_PACKAGE_NAME}_FOUND FALSE)
        return()
    endif ()
    include("${proteus_${type}_targets}")
endmacro()

if (proteus_comp_static)
    proteus_load_targets(static)
elseif (proteus_comp_shared)
    proteus_load_targets(shared)
elseif (DEFINED proteus_SHARED_LIBS AND proteus_SHARED_LIBS)
    proteus_load_targets(shared)
elseif (DEFINED proteus_SHARED_LIBS AND NOT proteus_SHARED_LIBS)
    proteus_load_targets(static)
elseif (PROTEUS_BUILD_SHARED)
    if (EXISTS "${proteus_shared_targets}")
        proteus_load_targets(shared)
    else ()
        proteus_load_targets(static)
    endif ()
else ()
    if (EXISTS "${proteus_static_targets}")
        proteus_load_targets(static)
    else ()
        proteus_load_targets(shared)
    endif ()
endif ()
