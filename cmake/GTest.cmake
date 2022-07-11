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

# While we can use an installed version of GTest and use find_package to get it,
# GTest's official guide recommends this approach of building the library with
# the same compile options as the executable being tested instead of linking
# against a precompiled library.
include(FetchContent)
FetchContent_Declare(
  googletest GIT_REPOSITORY "https://github.com/google/googletest"
  GIT_TAG "release-1.11.0"
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE INTERNAL "")

FetchContent_MakeAvailable(googletest)

# move all include directories to system directories
list(
  APPEND gtest_targets
         gtest
         gtest_main
         gmock
)
foreach(target ${gtest_targets})
  get_target_property(INCLUDE_DIRS ${target} INTERFACE_INCLUDE_DIRECTORIES)
  set_target_properties(
    ${target} PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${INCLUDE_DIRS}"
  )
endforeach()

# disable linting on GTest sources
list(APPEND linters CMAKE_CXX_INCLUDE_WHAT_YOU_USE CMAKE_CXX_CLANG_TIDY
            CMAKE_CXX_CPPLINT
)
foreach(linter ${linters})
  set_target_properties(gtest PROPERTIES ${linter} "")
  set_target_properties(gtest_main PROPERTIES ${linter} "")
  set_target_properties(gmock PROPERTIES ${linter} "")
endforeach()
