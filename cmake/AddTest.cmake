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

function(proteus_get_test_target target test)
  string(REGEX REPLACE "^${PROTEUS_TEST_ROOT}" "" BASE_PATH
                       ${CMAKE_CURRENT_SOURCE_DIR}
  )
  string(REGEX REPLACE "\/" "_" BASE_NAME ${BASE_PATH})

  # check for an extra argument to set the prefix. use 'test' by default
  list(LENGTH ARGN extra_count)
  if(extra_count GREATER 0)
    list(GET ARGN 0 prefix)
  elseif(extra_count GREATER 1)
    message(FATAL_ERROR "Cannot pass more than two arguments to this function")
  else()
    set(prefix "test")
  endif()

  set(${target} ${prefix}${BASE_NAME}-${test} PARENT_SCOPE)
endfunction()

function(_proteus_add_test test type)
  proteus_get_test_target(target ${test})

  add_executable(${target} test_${test}.cpp)
  if(${type} STREQUAL "unit")
    target_include_directories(
      ${target} BEFORE PRIVATE ${PROTEUS_TEST_INCLUDE_DIRS}
    )
    target_include_directories(${target} AFTER PRIVATE ${PROTEUS_INCLUDE_DIRS})
  elseif(${type} STREQUAL "system")
    target_include_directories(${target} BEFORE PRIVATE ${PROTEUS_INCLUDE_DIRS})
    target_include_directories(
      ${target} AFTER PRIVATE ${PROTEUS_TEST_INCLUDE_DIRS}
    )
    target_link_libraries(${target} PRIVATE proteus)
  else()
    message(FATAL_ERROR "Test type must be one of 'unit' or 'system'")
  endif()

  target_link_libraries(${target} PRIVATE gtest gtest_main)

  gtest_discover_tests(${target} DISCOVERY_TIMEOUT 30)
endfunction()

function(proteus_add_system_test test)
  _proteus_add_test(${test} "system")
endfunction()

function(proteus_add_unit_test test)
  _proteus_add_test(${test} "unit")
endfunction()

function(proteus_add_system_tests tests)
  foreach(test ${tests})
    proteus_add_system_test(${test})
  endforeach()
endfunction()

function(proteus_add_unit_tests tests tests_libs)
  foreach(test lib_str IN ZIP_LISTS tests tests_libs)

    string(REPLACE "~" ";" libs ${lib_str})

    proteus_add_unit_test(${test})
    proteus_get_test_target(target ${test})

    target_link_libraries(${target} PRIVATE ${libs})
  endforeach()
endfunction()
