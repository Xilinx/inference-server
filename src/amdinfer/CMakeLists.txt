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

# add all the source directories. These produce object libraries with the same
# names
set(client_targets clients core observation util)
set(server_targets batching buffers core observation servers util)
set(targets ${client_targets} ${server_targets})
list(REMOVE_DUPLICATES targets)

foreach(target ${targets})
  add_subdirectory(${target})
endforeach()
add_subdirectory(bindings)
add_subdirectory(models)
add_subdirectory(workers)

add_library(amdinfer SHARED ${targets})
target_link_libraries(amdinfer PRIVATE ${targets})

add_library(amdinfer-server SHARED ${server_targets})
target_link_libraries(amdinfer-server PRIVATE ${server_targets})

add_library(amdinfer-client SHARED ${client_targets})
target_link_libraries(amdinfer-client PRIVATE ${client_targets})

set(output_libraries amdinfer amdinfer-server amdinfer-client)
foreach(lib ${output_libraries})
  set_target_options(${lib})

  # add alias to keep build and install interface the same
  add_library(amdinfer::${lib} ALIAS ${lib})

  set_target_properties(
    ${lib} PROPERTIES VERSION ${amdinfer_VERSION}
                      SOVERSION ${amdinfer_VERSION_MAJOR}
  )

  set_target_properties(
    ${lib} PROPERTIES BUILD_RPATH ${PROJECT_BINARY_DIR}/src/amdinfer/workers
  )
endforeach()

# the compiled server binary
add_executable(amdinfer-server-exe main.cpp)
target_include_directories(amdinfer-server-exe PRIVATE ${AMDINFER_INCLUDE_DIRS})
target_link_options(amdinfer-server-exe PRIVATE "LINKER:-E")
set_target_options(amdinfer-server-exe)
set_target_properties(
  amdinfer-server-exe PROPERTIES OUTPUT_NAME amdinfer-server
)
target_link_libraries(amdinfer-server-exe PRIVATE amdinfer)

# ~~~
# include(GenerateExportHeader)
# generate_export_header(amdinfer EXPORT_FILE_NAME include/amdinfer/export.h)
# target_compile_definitions(
#   amdinfer PUBLIC
#   "$<$<NOT:$<BOOL:${AMDINFER_BUILD_SHARED}>>:AMDINFER_STATIC_DEFINE>"
# )
# target_include_directories(
#   amdinfer PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
# )
# ~~~

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(amdinfer_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/amdinfer"
    CACHE STRING "Path to amdinfer CMake files"
)

if(NOT SKBUILD)

  # cmake-format: off
  install(
    TARGETS amdinfer amdinfer-client amdinfer-server amdinfer-server-exe
    EXPORT amdinfer
    RUNTIME COMPONENT amdinfer_Runtime
    LIBRARY COMPONENT amdinfer_Runtime
    NAMELINK_COMPONENT amdinfer_Development
    ARCHIVE COMPONENT amdinfer_Development
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  )
  # cmake-format: on

  install(DIRECTORY "${amdinfer_SOURCE_DIR}/include/" TYPE INCLUDE
          COMPONENT amdinfer_Development
  )

  install(
    EXPORT amdinfer
    DESTINATION "${amdinfer_INSTALL_CMAKEDIR}"
    NAMESPACE amdinfer::
    FILE amdinfer-targets.cmake
    COMPONENT amdinfer_Development
  )

  write_basic_package_version_file(
    amdinferConfigVersion.cmake COMPATIBILITY SameMajorVersion
  )

  install(
    FILES "${PROJECT_SOURCE_DIR}/cmake/amdinferConfig.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/amdinferConfigVersion.cmake"
    DESTINATION "${amdinfer_INSTALL_CMAKEDIR}" COMPONENT amdinfer_Development
  )
else()
  # auditwheel needs the location where the compiled libraries are found, which
  # is CMAKE_CURRENT_BINARY_DIR. I can't install them because then they get
  # included in the Python package so this file holds this directory which gets
  # added to LD_LIBRARY_PATH before running auditwheel in cibuildwheel
  file(WRITE ${PROJECT_SOURCE_DIR}/tmp.cmake ${CMAKE_CURRENT_BINARY_DIR})
endif()

set(dependencies efsw::efsw protobuf::libprotobuf)
set(deb_dependencies "")

if(${AMDINFER_ENABLE_HTTP})
  list(APPEND dependencies Drogon::Drogon Trantor::Trantor)
  list(APPEND deb_dependencies libjsoncpp1)
endif()

if(${AMDINFER_ENABLE_VITIS})
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

  if(${AMDINFER_ENABLE_PYTHON_BINDINGS})
    find_package(Python COMPONENTS Interpreter Development)
    execute_process(
      COMMAND ${Python_EXECUTABLE} "-c" "import xir; print(xir.__file__)"
      OUTPUT_VARIABLE PY_XIR_PATH OUTPUT_STRIP_TRAILING_WHITESPACE
                                  COMMAND_ERROR_IS_FATAL ANY
    )
    list(APPEND deb_dependencies "libpython3")

    # install(FILES ${PY_XIR_PATH} TYPE LIB COMPONENT amdinfer_Runtime)
  endif()
endif()

if(${AMDINFER_ENABLE_AKS})
  list(APPEND dependencies aks::aks)
  list(APPEND deb_dependencies libboost-system1.65.1
              "libboost-filesystem1.65.1"
  )
endif()

if(${AMDINFER_ENABLE_TRACING})
  list(
    APPEND dependencies
           opentelemetry-cpp::exporter_otlp_http
           opentelemetry-cpp::resources
           opentelemetry-cpp::trace
           opentelemetry-cpp::common
           opentelemetry-cpp::exporter_otlp_http_client
  )
endif()

if(${AMDINFER_ENABLE_METRICS})
  list(APPEND dependencies prometheus-cpp::core)
endif()

# TODO(varunsh): shouldn't be installing the dependencies automatically. It can
# lead to duplicate/conflicting libraries
# if(NOT SKBUILD)
#   install(
#     IMPORTED_RUNTIME_ARTIFACTS
#     ${dependencies}
#     RUNTIME_DEPENDENCY_SET
#     amdinfer_Dependencies
#     LIBRARY
#     COMPONENT
#     amdinfer_Runtime
#     LIBRARY
#     DESTINATION
#     "${CMAKE_INSTALL_LIBDIR}"
#   )

#   install(RUNTIME_DEPENDENCY_SET amdinfer_Dependencies DESTINATION
#           "${CMAKE_INSTALL_LIBDIR}" COMPONENT amdinfer_Runtime
#   )
# endif()

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "libamdinfer")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AMD Inference Server")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.rst")
set(CPACK_PACKAGE_VENDOR "Advanced Micro Devices, Inc.")
set(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_CONTACT "varun.sharma@amd.com")
set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/amdinfer")
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
