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

add_executable(proteus-server-exe main.cpp)
target_include_directories(proteus-server-exe PRIVATE ${PROTEUS_INCLUDE_DIRS})
target_link_options(proteus-server-exe PRIVATE "LINKER:-E")
enable_ipo_on_target(proteus-server-exe)
set_target_properties(proteus-server-exe PROPERTIES OUTPUT_NAME proteus-server)

target_link_libraries(
  proteus-server-exe PRIVATE batching clients core helpers observation servers
)

add_subdirectory(batching)
add_subdirectory(bindings)
add_subdirectory(buffers)
add_subdirectory(client_operators)
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
  ${CLIENT_OPERATORS_OBJS}
  ${CLIENT_OBJS}
  ${CORE_OBJS}
  ${HELPER_OBJS}
  ${OBSERVATION_OBJS}
  ${SERVER_OBJS}
)
target_link_libraries(
  proteus PRIVATE batching clients core helpers observation servers
)

add_library(
  proteus-server ${TYPE} ${BATCHING_OBJS} ${CORE_OBJS} ${HELPER_OBJS}
                 ${OBSERVATION_OBJS} ${SERVER_OBJS}
)
target_link_libraries(
  proteus-server PRIVATE batching core helpers observation servers
)

add_library(
  proteus-client ${TYPE} ${CLIENT_OPERATORS_OBJS} ${CLIENT_OBJS} ${CORE_OBJS}
                 ${OBSERVATION_OBJS}
)
target_link_libraries(proteus-client PRIVATE clients core observation)

set(output_libraries proteus proteus-server proteus-client)
foreach(lib ${output_libraries})
  enable_ipo_on_target(${lib})

  # add alias to keep build and install interface the same
  add_library(proteus::${lib} ALIAS ${lib})

  set_target_properties(
    ${lib} PROPERTIES VERSION ${proteus_VERSION}
                      SOVERSION ${proteus_VERSION_MAJOR}
  )
endforeach()

# ~~~
# include(GenerateExportHeader)
# generate_export_header(proteus EXPORT_FILE_NAME include/proteus/export.h)
# target_compile_definitions(
#   proteus PUBLIC
#   "$<$<NOT:$<BOOL:${PROTEUS_BUILD_SHARED}>>:PROTEUS_STATIC_DEFINE>"
# )
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
  TARGETS proteus proteus-client proteus-server
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
  TARGETS proteus-server-exe
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

install(DIRECTORY "${proteus_SOURCE_DIR}/include/" TYPE INCLUDE
        COMPONENT proteus_Development
)

set(dependencies efsw::efsw protobuf::libprotobuf)
set(deb_dependencies "")

if(${PROTEUS_ENABLE_HTTP})
  list(APPEND dependencies Drogon::Drogon Trantor::Trantor)
  list(APPEND deb_dependencies libjsoncpp1)
endif()

if(${PROTEUS_ENABLE_VITIS})
  list(
    APPEND dependencies
           json-c::json-c
           rt-engine::rt-engine
           Sockpp::sockpp
           unilog::unilog
           vart::runner
           vart::util
           xir::xir
  )
  list(APPEND deb_dependencies libgoogle-glog0v5 xrt)

  if(${PROTEUS_ENABLE_PYTHON_BINDINGS})
    find_package(Python COMPONENTS Interpreter Development)
    execute_process(
      COMMAND ${Python_EXECUTABLE} "-c" "import xir; print(xir.__file__)"
      OUTPUT_VARIABLE PY_XIR_PATH OUTPUT_STRIP_TRAILING_WHITESPACE
                                  COMMAND_ERROR_IS_FATAL ANY
    )
    list(APPEND deb_dependencies "libpython3.6")

    install(FILES ${PY_XIR_PATH} TYPE LIB COMPONENT proteus_Runtime)
  endif()
endif()

if(${PROTEUS_ENABLE_AKS})
  list(APPEND dependencies aks::aks)
  list(APPEND deb_dependencies libboost-system1.65.1
              "libboost-filesystem1.65.1"
  )
endif()

if(${PROTEUS_ENABLE_TRACING})
  list(APPEND dependencies opentelemetry-cpp::jaeger_trace_exporter
              opentelemetry-cpp::resources opentelemetry-cpp::trace
              opentelemetry-cpp::common opentelemetry-cpp::http_client_curl
  )
endif()

if(${PROTEUS_ENABLE_METRICS})
  list(APPEND dependencies prometheus-cpp::core)
endif()

install(
  IMPORTED_RUNTIME_ARTIFACTS
  ${dependencies}
  RUNTIME_DEPENDENCY_SET
  proteus_Dependencies
  LIBRARY
  COMPONENT
  proteus_Runtime
  LIBRARY
  DESTINATION
  "${CMAKE_INSTALL_LIBDIR}"
)

install(RUNTIME_DEPENDENCY_SET proteus_Dependencies DESTINATION
        "${proteus_INSTALL_CMAKEDIR}" COMPONENT proteus_Runtime
)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "libproteus")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AMD Inference Server")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.rst")
set(CPACK_PACKAGE_VENDOR "Xilinx Inc.")
set(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_CONTACT "varunsh@amd.com")
set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/proteus")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.rst")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")

set(CPACK_DEB_COMPONENT_INSTALL OFF)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Varun Sharma")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
string(REPLACE ";" ", " deb_dependencies "${deb_dependencies}")
set(CPACK_DEBIAN_PACKAGE_DEPENDS ${deb_dependencies})
# set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)

include(CPack)
