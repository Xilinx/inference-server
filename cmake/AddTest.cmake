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

function(amdinfer_get_test_target target test)
  string(REGEX REPLACE "^${AMDINFER_TEST_ROOT}" "" BASE_PATH
                       ${CMAKE_CURRENT_SOURCE_DIR}
  )
  string(REGEX REPLACE "\/" "_" BASE_NAME ${BASE_PATH})

  # check for an extra argument to set the prefix. use 'test' by default
  list(LENGTH ARGN extra_count)
  if(extra_count GREATER 0)
    list(GET ARGN 0 prefix)
  elseif(extra_count GREATER 1)
    message(FATAL_ERROR "Cannot pass more than three args to this function")
  else()
    set(prefix "test")
  endif()

  set(${target} ${prefix}${BASE_NAME}-${test} PARENT_SCOPE)
endfunction()

function(_amdinfer_add_test test type)
  amdinfer_get_test_target(target ${test})

  add_executable(${target} test_${test}.cpp)
  if(${type} STREQUAL "unit")
    target_include_directories(
      ${target} BEFORE PRIVATE ${AMDINFER_TEST_INCLUDE_DIRS}
    )
    target_include_directories(${target} AFTER PRIVATE ${AMDINFER_INCLUDE_DIRS})
  elseif(${type} STREQUAL "system")
    target_include_directories(
      ${target} BEFORE PRIVATE ${AMDINFER_INCLUDE_DIRS}
    )
    target_include_directories(
      ${target} AFTER PRIVATE ${AMDINFER_TEST_INCLUDE_DIRS}
    )
    target_link_libraries(${target} PRIVATE amdinfer)
  else()
    message(FATAL_ERROR "Test type must be one of 'unit' or 'system'")
  endif()

  target_link_libraries(${target} PRIVATE test_main testing)

  gtest_discover_tests(${target} DISCOVERY_TIMEOUT 30)
endfunction()

function(amdinfer_add_system_test test)
  _amdinfer_add_test(${test} "system")
endfunction()

function(amdinfer_add_unit_test test)
  _amdinfer_add_test(${test} "unit")
endfunction()

function(amdinfer_add_system_tests tests)
  foreach(test ${tests})
    amdinfer_add_system_test(${test})
  endforeach()
endfunction()

function(amdinfer_add_unit_tests tests tests_libs)
  foreach(test lib_str IN ZIP_LISTS tests tests_libs)

    string(REPLACE " " "" libs_no_spaces ${lib_str})
    string(REPLACE "~" ";" libs ${libs_no_spaces})

    amdinfer_add_unit_test(${test})
    amdinfer_get_test_target(target ${test})

    target_link_libraries(${target} PRIVATE ${libs})
  endforeach()
endfunction()
