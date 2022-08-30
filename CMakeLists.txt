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

cmake_minimum_required(VERSION 3.21)

# parse Proteus version
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" ver)
string(REPLACE "\n" "" ver ${ver})
string(REGEX MATCHALL "([0-9]+)|-(.*)\\+" result ${ver})
list(GET result 0 ver_major)
list(GET result 1 ver_minor)
list(GET result 2 ver_patch)
list(LENGTH result result_len)
if(result_len EQUAL "4")
  list(GET result 3 ver_label)
  string(LENGTH ${ver_label} ver_label_len)
  string(SUBSTRING ${ver_label} 1 ${ver_label_len} ver_label)
  message(
    STATUS
      "Building version ${ver_major}.${ver_minor}.${ver_patch}-${ver_label}"
  )
else()
  set(ver_label "")
  message(STATUS "Building version ${ver_major}.${ver_minor}.${ver_patch}")
endif()

# set the project name
project(
  proteus
  VERSION ${ver_major}.${ver_minor}.${ver_patch}
  LANGUAGES CXX
  DESCRIPTION "Proteus inference library and server"
)

# Only do these if this is the main project, and not if it is included through
# add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set(CMAKE_CXX_LINKER_WRAPPER_FLAG "-Wl,")
  add_link_options("LINKER:--as-needed")
  add_link_options("LINKER:--no-undefined")
  add_link_options("LINKER:--no-allow-shlib-undefined")
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Coverage"
      CACHE STRING "Available build-types: Debug, Release and Coverage"
  )

  # set standard options
  set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -Wpedantic -pedantic-errors")
  set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
  set(CMAKE_CXX_FLAGS_RELEASE "-O2")

  set(PROTEUS_BUILD_TESTING_DEFAULT ON)
else()
  set(PROTEUS_BUILD_TESTING_DEFAULT OFF)
endif()

if(CMAKE_BUILD_TYPE MATCHES Coverage)
  include(CodeCoverage)
endif()
include(CMakeDependentOption)
include(CTest)
include(EnableIPO)
include(AddOption)

# check requirements
find_package(Drogon)
find_package(OpenCV)
find_package(opentelemetry-cpp CONFIG)
find_package(spdlog)
find_package(Threads REQUIRED)
# rocm, the toolkit used by migraphx
list(APPEND CMAKE_PREFIX_PATH /opt/rocm/hip /opt/rocm)
find_package(migraphx QUIET)
find_package(tfzendnn)
find_package(ptzendnn)
find_package(protobuf CONFIG)
find_package(absl CONFIG)
find_package(gRPC CONFIG)
find_package(Doxygen)
find_package(Sphinx)
find_package(efsw)
find_package(json-c)
find_package(prometheus-cpp)
find_package(sockpp)

find_package(xir QUIET)
find_package(vart CONFIG COMPONENTS runner util QUIET)
find_package(rt-engine QUIET)
find_package(unilog QUIET)
find_package(target-factory QUIET)
find_package(aks QUIET)
if(${xir_FOUND}
   AND ${vart_FOUND}
   AND ${rt-engine_FOUND}
   AND ${unilog_FOUND}
   AND ${target-factory_FOUND}
)
  set(PROTEUS_VITIS_FOUND ON)
  set(PROTEUS_AKS_FOUND ${aks_FOUND})
  message(STATUS "Enabling Vitis dependencies")
  if(aks_FOUND)
    message(STATUS "Enabling AKS dependencies")
  else()
    message(STATUS "AKS not found, disabling AKS dependencies")
  endif()
else()
  set(PROTEUS_VITIS_FOUND OFF)
  set(PROTEUS_AKS_FOUND OFF)
  message(STATUS "One or more Vitis dependencies not found. Disabling Vitis")
endif()

configure_file(
  ${PROJECT_SOURCE_DIR}/src/proteus/version.hpp.in
  ${PROJECT_SOURCE_DIR}/src/proteus/version.hpp
)

message(STATUS "Build Options:")
add_option("ENABLE_REST" "Enable the REST server" ON)
add_option("ENABLE_GRPC" "Enable the gRPC server" ${gRPC_FOUND})
add_option("ENABLE_METRICS" "Enable Prometheus metrics" ON)
add_option("ENABLE_LOGGING" "Enable logging" ${spdlog_FOUND})
add_option("ENABLE_TRACING" "Enable Jaeger tracing" ${opentelemetry-cpp_FOUND})
add_option("ENABLE_AKS" "Enable AKS dependencies" ${PROTEUS_AKS_FOUND})
add_option("ENABLE_VITIS" "Enable Vitis dependencies" ${PROTEUS_VITIS_FOUND})
add_option("BUILD_EXAMPLES" "Build examples" ON)
add_option("BUILD_SHARED" "Build Proteus as a shared library" ON)
add_option("BUILD_TESTING" "Build C++ tests" ${PROTEUS_BUILD_TESTING_DEFAULT})
add_option("ENABLE_IPO" "Enable interprocedural optimizations" OFF)
add_option("ENABLE_LINTING" "Enable build-time linting" OFF)
add_option("ENABLE_TFZENDNN" "Enable TF+ZenDNN worker" ${tfzendnn_FOUND})
add_option("ENABLE_PTZENDNN" "Enable PT+ZenDNN worker" ${ptzendnn_FOUND})
add_option("ENABLE_MIGRAPHX" "Enable migraphx worker" ${migraphx_FOUND})
add_option("ENABLE_PYTHON_BINDINGS" "Build Python bindings" ON)

message(STATUS "Derived Options:")
if(${PROTEUS_ENABLE_REST} OR ${PROTEUS_ENABLE_METRICS})
  add_derived_option("ENABLE_HTTP" ON)
else()
  add_derived_option("ENABLE_HTTP" OFF)
endif()

configure_file(
  src/proteus/build_options.hpp.in
  ${PROJECT_SOURCE_DIR}/include/proteus/build_options.hpp
)
configure_file(
  src/gui/src/build_options.ts.in
  ${PROJECT_SOURCE_DIR}/src/gui/src/build_options.ts
)
if(NOT EXISTS ${PROJECT_SOURCE_DIR}/tools/benchmark.yml)
  configure_file(
    tools/benchmark.yml.in ${PROJECT_SOURCE_DIR}/tools/benchmark.yml
  )
endif()

# include this after configuring all files due to the GLOB expression
include(ClangTools)

if(CMAKE_BUILD_TYPE MATCHES Coverage)
  append_coverage_compiler_flags()

  setup_target_for_coverage_fastcov(
    NAME
    coverage # New target name
    EXECUTABLE
    ${PROJECT_SOURCE_DIR}/tests/test.sh
    EXECUTABLE_ARGS
    --mode
    tests
    --build
    Coverage
    --load-before
    DEPENDENCIES
    proteus-server
    FASTCOV_ARGS
    "--include"
    ${PROJECT_SOURCE_DIR}
    "--exceptional-branch-coverage"
    BASE_DIRECTORY
    ${PROJECT_SOURCE_DIR}
    EXCLUDE
    ${PROJECT_SOURCE_DIR}/build
    ${PROJECT_SOURCE_DIR}/tests
    ${PROJECT_SOURCE_DIR}/external
    ${PROJECT_SOURCE_DIR}/examples
  )
else()
  message(STATUS "Not a coverage build, not configuring coverage measurement")
endif()

set(PROTEUS_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src
                         ${PROJECT_SOURCE_DIR}/include
)
set(PROTEUS_PUBLIC_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include)

if(${PROTEUS_ENABLE_LINTING})
  enable_cxx_linting()
endif()

if(${Doxygen_FOUND} AND ${Sphinx_FOUND})
  message(STATUS "Doxygen and Sphinx found. Building documentation")
  add_subdirectory(docs)
else()
  message(STATUS "Doxygen or Sphinx not found. Skipping building documentation")
endif()

add_subdirectory(external)
add_subdirectory(src/proteus)
if(${PROTEUS_BUILD_EXAMPLES})
  add_subdirectory(examples/cpp)
endif()
if(${PROTEUS_BUILD_TESTING})
  include(GTest)
  include(GoogleTest)
  add_subdirectory(tests)
endif()
