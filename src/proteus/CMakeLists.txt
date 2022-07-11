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

add_executable(proteus-server main.cpp)
target_include_directories(proteus-server PRIVATE ${PROTEUS_INCLUDE_DIRS})
target_link_options(proteus-server PRIVATE "LINKER:-E")
enable_ipo_on_target(proteus-server)

target_link_libraries(
  proteus-server PRIVATE batching clients core helpers observation servers
)

add_subdirectory(batching)
add_subdirectory(bindings)
add_subdirectory(buffers)
add_subdirectory(clients)
add_subdirectory(core)
add_subdirectory(helpers)
add_subdirectory(observation)
add_subdirectory(servers)
add_subdirectory(workers)

# Build and install the proteus library. The dual static/shared build process is
# guided by: https://github.com/alexreinking/SharedStaticStarter and its
# accompanying article.

if(${PROTEUS_BUILD_SHARED})
  set(type shared)
else()
  set(type static)
endif()
string(TOUPPER ${type} TYPE)

add_library(
  proteus
  ${TYPE}
  ${BATCHING_OBJS}
  ${CLIENT_OBJS}
  ${CORE_OBJS}
  ${HELPER_OBJS}
  ${OBSERVATION_OBJS}
  ${SERVER_OBJS}
)
enable_ipo_on_target(proteus)
target_link_libraries(
  proteus PRIVATE batching clients core helpers observation servers
)

# add alias to keep build and install interface the same
add_library(proteus::proteus ALIAS proteus)

set_target_properties(
  proteus PROPERTIES VERSION ${proteus_VERSION}
                     SOVERSION ${proteus_VERSION_MAJOR}
)

# ~~~
# include(GenerateExportHeader)
# generate_export_header(proteus EXPORT_FILE_NAME include/proteus/export.h)
# target_compile_definitions(
#   proteus PUBLIC "$<$<NOT:$<BOOL:${PROTEUS_BUILD_SHARED}>>:PROTEUS_STATIC_DEFINE>")
# target_include_directories(
#   proteus PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
# )
# ~~~

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(proteus_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/proteus"
    CACHE STRING "Path to proteus CMake files"
)

# cmake-format: off
install(
  TARGETS proteus
  EXPORT proteus_Targets
  RUNTIME COMPONENT proteus_Runtime
  LIBRARY COMPONENT proteus_Runtime
  NAMELINK_COMPONENT proteus_Development
  ARCHIVE COMPONENT proteus_Development
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
# cmake-format: on

install(
  TARGETS proteus-server
  EXPORT proteus_Server
  RUNTIME COMPONENT proteus_Runtime
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(DIRECTORY "${proteus_SOURCE_DIR}/include/" TYPE INCLUDE
        COMPONENT proteus_Development
)

install(
  EXPORT proteus_Targets
  DESTINATION "${proteus_INSTALL_CMAKEDIR}"
  NAMESPACE proteus::
  FILE proteus-${type}-targets.cmake
  COMPONENT proteus_Development
)

write_basic_package_version_file(
  proteusConfigVersion.cmake COMPATIBILITY SameMajorVersion
)

install(FILES "${PROJECT_SOURCE_DIR}/cmake/proteusConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/proteusConfigVersion.cmake"
        DESTINATION "${proteus_INSTALL_CMAKEDIR}" COMPONENT proteus_Development
)
