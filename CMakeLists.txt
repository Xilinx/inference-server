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

cmake_minimum_required(VERSION 3.19.4) # minimum is at least 3.12...

# parse Proteus version
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" ver)
string(REPLACE "\n" "" ver ${ver})
string(REGEX MATCHALL "([0-9]+)|-(.*)" result ${ver})
list(GET result 0 ver_major)
list(GET result 1 ver_minor)
list(GET result 2 ver_patch)
list(LENGTH result result_len)
if(result_len EQUAL "4")
    list(GET result 3 ver_label)
    string(LENGTH ${ver_label} ver_label_len)
    string(SUBSTRING ${ver_label} 1 ${ver_label_len} ver_label)
    message(STATUS "Building Proteus version ${ver_major}.${ver_minor}.${ver_patch}-${ver_label}")
else()
    set(ver_label "")
    message(STATUS "Building Proteus version ${ver_major}.${ver_minor}.${ver_patch}")
endif()

# set the project name
project(proteus
    VERSION ${ver_major}.${ver_minor}.${ver_patch}
    LANGUAGES CXX
    DESCRIPTION "Proteus inference library and server"
)

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CMAKE_CXX_LINKER_WRAPPER_FLAG "-Wl,")
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Coverage" CACHE STRING
        "Available build-types: Debug, Release and Coverage")

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

# check requirements
find_package(Drogon)
find_package(OpenCV)
find_package(opentelemetry-cpp CONFIG)
find_package(spdlog)
find_package(Threads REQUIRED)

find_package(xir QUIET)
find_package(vart QUIET)
find_package(rt-engine QUIET)
find_package(unilog QUIET)
find_package(target-factory QUIET)
find_package(aks QUIET)
if(${xir_FOUND} AND ${vart_FOUND} AND ${rt-engine_FOUND} AND ${unilog_FOUND} AND ${target-factory_FOUND})
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
    message(STATUS "One or more Vitis dependencies weren't found. Disabling all Vitis dependencies")
endif()

configure_file(${PROJECT_SOURCE_DIR}/src/proteus/version.hpp.in ${PROJECT_SOURCE_DIR}/src/proteus/version.hpp)

option(PROTEUS_ENABLE_REST "Enable the REST server" ON)
option(PROTEUS_ENABLE_METRICS "Enable Prometheus metrics" ON)
option(PROTEUS_ENABLE_LOGGING "Enable logging" ${spdlog_FOUND})
option(PROTEUS_ENABLE_TRACING "Enable Jaeger tracing" ${opentelemetry-cpp_FOUND})
option(PROTEUS_ENABLE_AKS "Enable AKS dependencies" ${PROTEUS_AKS_FOUND})
option(PROTEUS_ENABLE_VITIS "Enable Vitis dependencies" ${PROTEUS_VITIS_FOUND})
option(PROTEUS_BUILD_EXAMPLES "Build examples" ON)
option(PROTEUS_BUILD_SHARED "Build Proteus as a shared library" ON)
option(PROTEUS_BUILD_TESTING "Build C++ tests" ${PROTEUS_BUILD_TESTING_DEFAULT})

if(${PROTEUS_ENABLE_REST} OR ${PROTEUS_ENABLE_METRICS})
    set(PROTEUS_ENABLE_HTTP ON)
else()
    set(PROTEUS_ENABLE_HTTP OFF)
endif()

set(PROTEUS_DEFAULT_HTTP_PORT 8998)

message(STATUS "Build Options:")
message(STATUS "  PROTEUS_ENABLE_REST: " ${PROTEUS_ENABLE_REST})
message(STATUS "  PROTEUS_ENABLE_METRICS: " ${PROTEUS_ENABLE_METRICS})
message(STATUS "  PROTEUS_ENABLE_LOGGING: " ${PROTEUS_ENABLE_LOGGING})
message(STATUS "  PROTEUS_ENABLE_TRACING: " ${PROTEUS_ENABLE_TRACING})
message(STATUS "  PROTEUS_ENABLE_AKS: " ${PROTEUS_ENABLE_AKS})
message(STATUS "  PROTEUS_ENABLE_VITIS: " ${PROTEUS_ENABLE_VITIS})
message(STATUS "  PROTEUS_BUILD_EXAMPLES: " ${PROTEUS_BUILD_EXAMPLES})
message(STATUS "  PROTEUS_BUILD_SHARED: " ${PROTEUS_BUILD_SHARED})
message(STATUS "  PROTEUS_BUILD_TESTING: " ${PROTEUS_BUILD_TESTING})
message(STATUS "Derived Options:")
message(STATUS "  PROTEUS_ENABLE_REST: " ${PROTEUS_ENABLE_REST})
message(STATUS "  PROTEUS_DEFAULT_HTTP_PORT: " ${PROTEUS_DEFAULT_HTTP_PORT})

configure_file(src/proteus/build_options.hpp.in ${PROJECT_SOURCE_DIR}/include/proteus/build_options.hpp)
configure_file(src/gui/src/build_options.ts.in ${PROJECT_SOURCE_DIR}/src/gui/src/build_options.ts)
if(NOT EXISTS ${PROJECT_SOURCE_DIR}/tools/benchmark.yml)
    configure_file(tools/benchmark.yml.in ${PROJECT_SOURCE_DIR}/tools/benchmark.yml)
endif()

# include this after configuring all files due to the GLOB expression
include(ClangTools)

if(CMAKE_BUILD_TYPE MATCHES Coverage)
    append_coverage_compiler_flags()

    setup_target_for_coverage_fastcov(
        NAME coverage # New target name
        EXECUTABLE ${PROJECT_SOURCE_DIR}/tests/test.sh
        EXECUTABLE_ARGS --mode all --build Coverage -k "not facedetect"
        DEPENDENCIES proteus-server
        FASTCOV_ARGS "--include" ${PROJECT_SOURCE_DIR} "--exceptional-branch-coverage"
        BASE_DIRECTORY ${PROJECT_SOURCE_DIR}
        EXCLUDE ${PROJECT_SOURCE_DIR}/build ${PROJECT_SOURCE_DIR}/tests
    )
else()
    message(STATUS "Not a coverage build, not configuring coverage measurement")
endif()

set(PROTEUS_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/include)
set(PROTEUS_PUBLIC_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include)

find_package(Doxygen)
find_package(Sphinx)
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
    add_subdirectory(tests/cpp)
endif()
