# Copyright 2022 Xilinx, Inc.
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

set(docstrings_filepath ${CMAKE_CURRENT_BINARY_DIR}/auto_docstrings.hpp)
set(AMDINFER_PYTHON_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}
                                 ${Python_INCLUDE_DIRS}
)

find_package(Pybind11Mkdoc)
if(${Pybind11Mkdoc_FOUND})
  file(GLOB_RECURSE ALL_PUBLIC_HEADER_FILES CONFIGURE_DEPENDS
       ${AMDINFER_PUBLIC_INCLUDE_DIRS}/*.hpp
  )

  # get the "-std=c++xx" flag to pass to pybind11-mkdoc
  set(CXX_STANDARD_FLAG
      ${CMAKE_CXX${CMAKE_CXX_STANDARD}_STANDARD_COMPILE_OPTION}
  )

  add_custom_command(
    OUTPUT ${docstrings_filepath}
    COMMAND
      pybind11-mkdoc -o ${docstrings_filepath} ${ALL_PUBLIC_HEADER_FILES}
      ${CXX_STANDARD_FLAG} -I${AMDINFER_PUBLIC_INCLUDE_DIRS}
      -I${HALF_INCLUDE_DIRS}
    DEPENDS ${ALL_PUBLIC_HEADER_FILES}
  )
else()
  configure_file(helpers/auto_docstrings.hpp.in ${docstrings_filepath})
endif()

add_custom_target(pybind11Mkdoc DEPENDS ${docstrings_filepath})

set(targets clients core pre_post servers)
if(${AMDINFER_BUILD_TESTING})
  list(APPEND targets testing)
endif()

foreach(target ${targets})
  add_subdirectory(${target})
endforeach()

set(new_targets ${targets})
list(
  TRANSFORM new_targets
            REPLACE
            "^(.+)$"
            "\\1_py"
)

# add this dummy file to add a dependency on the Pybind11 target so it runs if
# the amdinfer package isn't installed in pip
set(dummy_new_file ${CMAKE_CURRENT_BINARY_DIR}/src/main.cpp)
if(NOT EXISTS ${dummy_new_file})
  file(WRITE ${dummy_new_file} "")
endif()
execute_process(
  COMMAND sh -c "python3 -c 'import amdinfer'" RESULT_VARIABLE Result
  OUTPUT_QUIET ERROR_QUIET
)
if(${Result})
  # touch the file to modify it if pip fails to find the package
  file(TOUCH ${dummy_new_file})
endif()

pybind11_add_module(amdinfer_py MODULE amdinfer.cpp ${dummy_new_file})
target_include_directories(amdinfer_py PRIVATE ${AMDINFER_INCLUDE_DIRS})
target_link_libraries(
  amdinfer_py PRIVATE amdinfer ${Python_LIBRARIES} ${new_targets}
)
set_target_properties(amdinfer_py PROPERTIES OUTPUT_NAME _amdinfer)
set_target_options(amdinfer_py)

if(UNIX AND NOT APPLE)
  set_target_properties(amdinfer_py PROPERTIES INSTALL_RPATH "$ORIGIN")
elseif(APPLE)
  set_target_properties(amdinfer_py PROPERTIES INSTALL_RPATH "@loader_path")
endif()

# if building a wheel, the CMake build there sets this variable so we can detect
# here whether CMake is running in the regular build or as part of the wheel
# building run
get_target_property(output_directory amdinfer_py LIBRARY_OUTPUT_DIRECTORY)
if(NOT output_directory)
  set(output_directory ${CMAKE_CURRENT_BINARY_DIR})
endif()

find_package(Pybind11Stubgen)
if(${Pybind11Stubgen_FOUND})
  add_custom_command(
    TARGET amdinfer_py
    POST_BUILD
    COMMAND PYTHONPATH=${output_directory} pybind11-stubgen -o
            ${output_directory} _amdinfer --no-setup-py
    COMMAND mkdir -p ${output_directory}/src/amdinfer
    COMMAND cp -rf ${output_directory}/_amdinfer-stubs/*
            ${output_directory}/src/amdinfer
    COMMAND rm -rf ${output_directory}/_amdinfer-stubs
  )
else()
  message(
    STATUS "Pybind11Stubgen executable not found, not generating Python stubs"
  )
endif()

if(NOT SKBUILD)
  # copy over Python package source files to the build tree
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/setup.py
       DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
  )
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src
       DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
  )

  execute_process(
    COMMAND ${Python_EXECUTABLE} -m site --user-site
    OUTPUT_VARIABLE PY_USER_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE
                    COMMAND_ERROR_IS_FATAL ANY
  )

  add_custom_command(
    TARGET amdinfer_py
    POST_BUILD
    COMMAND pip install --prefix ~/.local ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/*.so
            ${PY_USER_SITE_PACKAGES}/amdinfer
  )
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# cmake-format: off
# it's possible that the SKBUILD version doesn't need the library installed at
# . but it needs to be verified. If so, this if-else can be removed
if(SKBUILD)
  install(
    TARGETS amdinfer_py
    LIBRARY COMPONENT amdinfer_Runtime
    LIBRARY DESTINATION .
  )
else()
  install(
    TARGETS amdinfer_py
    LIBRARY COMPONENT amdinfer_Runtime
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  )
endif()
# cmake-format: on

# we need to run this after the pip install above
# if(${Pybind11Stubgen_FOUND})
#   file(GLOB lib_exists ${output_directory}/_amdinfer*.so)
#   if(EXISTS ${lib_exists})
#     add_custom_command(
#       TARGET amdinfer_py POST_BUILD
#       COMMAND PYTHONPATH=${output_directory} pybind11-stubgen -o
#               ${output_directory} amdinfer --no-setup-py
#     )
#   endif()
# endif()
