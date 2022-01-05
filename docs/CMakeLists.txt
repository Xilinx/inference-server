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

# Inspired by: https://devblogs.microsoft.com/cppblog/clear-functional-c-documentation-with-sphinx-breathe-doxygen-cmake/

find_program(SYMLINKS_EXECUTABLE symlinks REQUIRED)

# Get all project files
file(GLOB_RECURSE ALL_SOURCE_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/proteus/*.cpp
    ${PROJECT_SOURCE_DIR}/src/proteus/*.hpp
    ${PROJECT_SOURCE_DIR}/include/proteus/*.hpp
)

# This will be the main output of our command
set(DOXYGEN_INDEX_FILE ${CMAKE_CURRENT_SOURCE_DIR}/xml/index.xml)
set(DOXYGEN_INPUT_DIRS ${PROJECT_SOURCE_DIR}/src/proteus ${PROJECT_SOURCE_DIR}/include/proteus)
set(DOXYGEN_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include/proteus)
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

string (REPLACE ";" "\" \"" DOXYGEN_INPUT_DIR "${DOXYGEN_INPUT_DIRS}")

# Replace variables inside @@ with the current values
configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

# intermittent failures if we only create the parent directory
file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR}/html)
file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR}/xml)

# Only regenerate Doxygen when the Doxyfile or files change
add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
    DEPENDS ${ALL_SOURCE_FILES}
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
    MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
    COMMENT "Generating docs"
    VERBATIM
)

# Nice named target so we can run the job easily
add_custom_target(doxygen DEPENDS ${DOXYGEN_INDEX_FILE})

add_custom_command(TARGET doxygen POST_BUILD
    COMMENT "Linking ${DOXYGEN_OUTPUT_DIR} to Proteus gui at ${PROJECT_SOURCE_DIR}/src/gui/public/"
    COMMAND rm -rf ${PROJECT_SOURCE_DIR}/src/gui/public/doxygen
    COMMAND cp -rfs ${DOXYGEN_OUTPUT_DIR} ${PROJECT_SOURCE_DIR}/src/gui/public/
    COMMAND symlinks -rc ${PROJECT_SOURCE_DIR}/src/gui/public/doxygen
    COMMENT "Linking ${DOXYGEN_OUTPUT_DIR} to global docs at ${PROJECT_SOURCE_DIR}/build/docs/doxygen"
    COMMAND mkdir -p ${PROJECT_SOURCE_DIR}/build/docs
    COMMAND rm -rf ${PROJECT_SOURCE_DIR}/build/docs/doxygen
    COMMAND cp -rfs ${DOXYGEN_OUTPUT_DIR} ${PROJECT_SOURCE_DIR}/build/docs
    COMMAND symlinks -rc ${PROJECT_SOURCE_DIR}/build/docs/doxygen
)

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

# Only regenerate Sphinx when:
# - Doxygen has rerun
# - Our doc files have been updated
# - The Sphinx config has been updated
add_custom_command(OUTPUT ${SPHINX_INDEX_FILE}
    COMMAND
        ${SPHINX_EXECUTABLE} -b html
        # Tell Breathe where to find the Doxygen output
        -Dbreathe_projects.Proteus=${DOXYGEN_OUTPUT_DIR}/xml
        -E -a
    ${SPHINX_SOURCE} ${SPHINX_BUILD}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS
    # Other docs files you want to track should go here (or in some variable)
    ${CMAKE_CURRENT_SOURCE_DIR}/index.rst
    ${DOXYGEN_INDEX_FILE}
    MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
    COMMENT "Generating documentation with Sphinx")

# Nice named target so we can run the job easily
add_custom_target(sphinx DEPENDS ${SPHINX_INDEX_FILE})
add_custom_command(TARGET sphinx POST_BUILD
    COMMENT "Linking ${SPHINX_BUILD} to Proteus gui at ${PROJECT_SOURCE_DIR}/src/gui/public/"
    COMMAND rm -rf ${PROJECT_SOURCE_DIR}/src/gui/public/sphinx
    COMMAND cp -rfs ${SPHINX_BUILD} ${PROJECT_SOURCE_DIR}/src/gui/public/
    COMMAND symlinks -rc ${PROJECT_SOURCE_DIR}/src/gui/public/sphinx
    COMMENT "Linking ${SPHINX_BUILD} to global docs at ${PROJECT_SOURCE_DIR}/build/docs/sphinx"
    COMMAND rm -rf ${PROJECT_SOURCE_DIR}/build/docs/sphinx
    COMMAND cp -rfs ${SPHINX_BUILD} ${PROJECT_SOURCE_DIR}/build/docs
    COMMAND symlinks -rc ${PROJECT_SOURCE_DIR}/build/docs/sphinx
)

add_custom_target(
  confluence
  COMMAND ${SPHINX_EXECUTABLE} -b confluence
  ${SPHINX_SOURCE} ${SPHINX_BUILD}
  -E -a
)

add_custom_target(
  doctest
  COMMAND ${SPHINX_EXECUTABLE} -b linkcheck
  ${SPHINX_SOURCE} ${SPHINX_BUILD}
)
