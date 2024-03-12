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

cmake_minimum_required(VERSION 3.21)

# parse the VERSION file in the current directory and extract it into variables
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

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(AddOption)

# these need to be before the call to project() to configure the VCPKG features
# correctly

# these options get added as AMDINFER_* e.g. AMDINFER_BUILD_TESTING
message(STATUS "Build Options:")
add_option("ENABLE_HTTP" "Enable the HTTP server" ON)
add_option("ENABLE_GRPC" "Enable the gRPC server" ON)
add_option("ENABLE_METRICS" "Enable Prometheus metrics" ON)
add_option("ENABLE_LOGGING" "Enable logging" ON)
add_option("ENABLE_TRACING" "Enable OTLP tracing" OFF)
add_option("BUILD_EXAMPLES" "Build examples" ON)
add_option("BUILD_APPS" "Build apps" ON)
add_option("BUILD_SHARED" "Build AMDinfer as a shared library" ON)
add_option("ENABLE_IPO" "Enable interprocedural optimizations" OFF)
add_option("ENABLE_LINTING" "Enable build-time linting" OFF)
add_option("ENABLE_PYTHON_BINDINGS" "Build Python bindings" ON)
add_option("INSTALL" "Set when invoking installation" OFF)

add_option_env("ENABLE_PTZENDNN" "Enable PT+ZenDNN backend" OFF)
add_option_env("ENABLE_TFZENDNN" "Enable TF+ZenDNN backend" OFF)
add_option_env("ENABLE_MIGRAPHX" "Enable migraphx backend" OFF)
add_option_env("ENABLE_ROCAL" "Enable rocAL backend" OFF)
add_option_env("ENABLE_AKS" "Enable AKS dependencies" OFF)
add_option_env("ENABLE_VITIS" "Enable Vitis dependencies" OFF)

# override tracing option and disable it if building Python bindings
if(SKBUILD)
  set(AMDINFER_ENABLE_TRACING OFF)
endif()

if(AMDINFER_ENABLE_HTTP)
  list(APPEND VCPKG_MANIFEST_FEATURES "http")
endif()

if(AMDINFER_ENABLE_METRICS)
  list(APPEND VCPKG_MANIFEST_FEATURES "metrics")
endif()

if(AMDINFER_ENABLE_LOGGING)
  list(APPEND VCPKG_MANIFEST_FEATURES "logging")
endif()

if(AMDINFER_ENABLE_TRACING)
  list(APPEND VCPKG_MANIFEST_FEATURES "tracing")
endif()

if(AMDINFER_ENABLE_AKS OR AMDINFER_ENABLE_VITIS)
  list(APPEND VCPKG_MANIFEST_FEATURES "vitis")
endif()

if(AMDINFER_ENABLE_ROCAL)
  list(APPEND VCPKG_MANIFEST_FEATURES "rocal")
endif()

list(APPEND VCPKG_MANIFEST_FEATURES "testing")

# In CMake 3.27+, find_package uses <PACKAGE_NAME>_ROOT variables. We're using
# AKS_ROOT in the environment currently.
if(${CMAKE_VERSION} VERSION_GREATER "3.27")
  cmake_policy(SET CMP0144 OLD)
endif()

# set the project name
project(
  amdinfer
  VERSION ${ver_major}.${ver_minor}.${ver_patch}
  LANGUAGES C CXX
  DESCRIPTION "AMDinfer inference library and server"
)

# Only do these if this is the main project, and not if it is included through
# add_subdirectory
if(PROJECT_IS_TOP_LEVEL)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set(CMAKE_CXX_LINKER_WRAPPER_FLAG "-Wl,")
  add_link_options("LINKER:--as-needed")
  # cannot use linker options like --no-undefined and --no-allow-shlib-undefined
  # because for manylinux builds, the Python extension is intentionally not
  # linked against Python at compile time so we need to allow undefined symbols.
  # The workers are also not fully linked either.
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Coverage;RelWithDebInfo"
      CACHE STRING "Available build-types: Debug, Release and Coverage"
  )
endif()

if(CMAKE_BUILD_TYPE MATCHES Coverage)
  include(CodeCoverage)
endif()
include(CMakeDependentOption)
include(CTest)
include(CheckIncludeFileCXX)
# these are defined locally in ./cmake/*
include(SetTargetOptions)
include(AddTargets)

# check requirements
# there's a weird error when using vcpkg if this is omitted though it should be
# an internal dependency of drogon (possibly reported here:
# https://github.com/drogonframework/drogon/issues/1422)
find_package(c-ares)
find_package(nlohmann_json CONFIG)

find_package(unofficial-b64 CONFIG)
find_package(cxxopts CONFIG REQUIRED)
find_package(GTest CONFIG)
find_package(Drogon CONFIG)
find_package(OpenCV)
find_package(opentelemetry-cpp CONFIG)
find_package(spdlog)
find_package(Threads REQUIRED)
# rocm, the toolkit used by migraphx
list(APPEND CMAKE_PREFIX_PATH /opt/rocm/hip /opt/rocm)
find_package(migraphx QUIET)
find_package(rocal QUIET)
find_package(tfzendnn)
find_package(ptzendnn)
find_package(Protobuf CONFIG)
find_package(absl CONFIG)
find_package(gRPC CONFIG)
find_package(Doxygen)
find_package(Sphinx)
find_package(efsw)
find_package(json-c QUIET)
find_package(prometheus-cpp)
find_package(sockpp QUIET)
find_package(tomlplusplus CONFIG)
find_path(HALF_INCLUDE_DIRS "half/half.hpp")
if(NOT HALF_INCLUDE_DIRS)
  message(FATAL_ERROR "half could not be included, required for FP16 support")
endif()
include_directories(SYSTEM ${HALF_INCLUDE_DIRS})

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
  set(AMDINFER_VITIS_FOUND ON)
  set(AMDINFER_AKS_FOUND ${aks_FOUND})
  message(STATUS "Enabling Vitis dependencies")
  if(aks_FOUND)
    message(STATUS "Enabling AKS dependencies")
  else()
    message(STATUS "AKS not found, disabling AKS dependencies")
  endif()
else()
  set(AMDINFER_VITIS_FOUND OFF)
  set(AMDINFER_AKS_FOUND OFF)
  message(STATUS "One or more Vitis dependencies not found. Disabling Vitis")
endif()

configure_file(
  ${PROJECT_SOURCE_DIR}/src/amdinfer/version.hpp.in
  ${PROJECT_SOURCE_DIR}/src/amdinfer/version.hpp
)

add_option("BUILD_TESTING" "Build C++ tests" ${PROJECT_IS_TOP_LEVEL})

configure_file(
  src/amdinfer/build_options.hpp.in
  ${PROJECT_SOURCE_DIR}/include/amdinfer/build_options.hpp
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
    python3
    EXECUTABLE_ARGS
    ${PROJECT_SOURCE_DIR}/tests/test.py
    --mode
    tests
    --build
    Coverage
    --load-before
    DEPENDENCIES
    amdinfer-server
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
    ${PROJECT_SOURCE_DIR}/src/amdinfer/bindings
  )
else()
  message(STATUS "Not a coverage build, not configuring coverage measurement")
endif()

set(AMDINFER_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src
                          ${PROJECT_SOURCE_DIR}/include
)
set(AMDINFER_PUBLIC_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include)
set(AMDINFER_TEST_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/tests/src)

if(${AMDINFER_ENABLE_LINTING})
  enable_cxx_linting()
endif()

if(${Doxygen_FOUND} AND ${Sphinx_FOUND})
  message(STATUS "Doxygen and Sphinx found. Building documentation")
  add_subdirectory(docs)
else()
  message(STATUS "Doxygen or Sphinx not found. Skipping building documentation")
endif()

add_subdirectory(external)
add_subdirectory(src/amdinfer)
if(${AMDINFER_BUILD_APPS})
  add_subdirectory(apps)
endif()
if(${AMDINFER_BUILD_EXAMPLES})
  add_subdirectory(examples)
endif()
if(${AMDINFER_BUILD_TESTING})
  include(GoogleTest)
  add_subdirectory(tests)
endif()
