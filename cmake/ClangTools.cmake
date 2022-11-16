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

# get all project files
file(
  GLOB_RECURSE
  ALL_SOURCE_FILES
  CONFIGURE_DEPENDS
  ${PROJECT_SOURCE_DIR}/examples/cpp/*.cpp
  ${PROJECT_SOURCE_DIR}/include/amdinfer/*.hpp
  ${PROJECT_SOURCE_DIR}/src/amdinfer/*.cpp
  ${PROJECT_SOURCE_DIR}/src/amdinfer/*.hpp
  ${PROJECT_SOURCE_DIR}/tests/*.cpp
  ${PROJECT_SOURCE_DIR}/tests/*.hpp
)

find_program(CLANG_FORMAT_EXECUTABLE clang-format)
if(CLANG_FORMAT_EXECUTABLE)
  message(STATUS "clang-format found. Adding clangformat make target")
  add_custom_target(
    clangformat COMMAND clang-format -style=file -i ${ALL_SOURCE_FILES}
  )
else()
  message(STATUS "clang-format not found. Skipping clangformat make target")
endif()

find_program(CLANG_TIDY_EXECUTABLE clang-tidy)
if(CLANG_TIDY_EXECUTABLE)
  message(STATUS "clang-tidy found. Adding clangtidy make target")
  # there's an opencv bug in the current version that fails in clangtidy. Adding
  # the extra define addresses it.
  add_custom_target(
    clangtidy COMMAND clang-tidy -format-style=file -p=${CMAKE_BINARY_DIR}
                      --extra-arg "-DCV_STATIC_ANALYSIS=0" ${ALL_SOURCE_FILES}
  )
else()
  message(STATUS "clang-tidy not found. Skipping clangtidy make target")
endif()

find_program(IWYU_EXECUTABLE include-what-you-use)
find_program(IWYU_SCRIPT_EXECUTABLE iwyu_tool.py)
if(IWYU_EXECUTABLE AND IWYU_SCRIPT_EXECUTABLE)
  message(STATUS "iwyu found. Adding iwyu make target")
  add_custom_target(
    iwyu COMMAND python3 /usr/local/bin/iwyu_tool.py -p ${CMAKE_BINARY_DIR} --
                 -Xiwyu --mapping_file=${PROJECT_SOURCE_DIR}/tools/.iwyu.json
  )
else()
  message(STATUS "iwyu not found. Skipping iwyu make target")
endif()

function(enable_cxx_linting)
  message(STATUS "Enabling build-time linting")

  set(CMAKE_LINK_WHAT_YOU_USE TRUE PARENT_SCOPE)

  find_program(IWYU_PATH NAMES include-what-you-use iwyu)
  if(IWYU_PATH)
    message(STATUS "Found IWYU")
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE
        ${IWYU_PATH} -Xiwyu
        --mapping_file=${PROJECT_SOURCE_DIR}/tools/.iwyu.json -Xiwyu --cxx17n
        PARENT_SCOPE
    )
  endif()

  find_program(CLANG_TIDY_PATH NAMES clang-tidy)
  if(CLANG_TIDY_PATH)
    message(STATUS "Found clang-tidy")
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_PATH} -format-style=file --extra-arg
                             "-DCV_STATIC_ANALYSIS=0" PARENT_SCOPE
    )
  endif()

  find_program(CPPLINT_PATH NAMES cpplint)
  if(CPPLINT_PATH)
    message(STATUS "Found cpplint")
    set(CMAKE_CXX_CPPLINT ${CPPLINT_PATH} PARENT_SCOPE)
  endif()
endfunction()

function(disable_target_linting target)
  set_target_properties(
    ${target}
    PROPERTIES CXX_INCLUDE_WHAT_YOU_USE ""
               CXX_CLANG_TIDY ""
               CXX_CPPLINT ""
               LINK_WHAT_YOU_USE FALSE
  )
endfunction()
