# Copyright 2023 Advanced Micro Devices, Inc.
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

cmake_minimum_required(VERSION 3.21)

project(
  app-mlcommons
  VERSION 0.1.0
  LANGUAGES C CXX
  DESCRIPTION "AMDinfer MLCommons App"
)

if(PROJECT_IS_TOP_LEVEL)
  find_package(amdinfer REQUIRED)
  find_package(Threads REQUIRED)
endif()

check_include_file_cxx(mlcommons/loadgen/loadgen.h LOADGEN_INCLUDE)
if(NOT LOADGEN_INCLUDE)
  message(
    WARNING
      "Skipping app: mlcommons ('mlcommons/loadgen/loadgen.h' could not be included)"
  )
else()
  message(STATUS "Building apps: mlcommons")
  add_subdirectory(src)
endif()