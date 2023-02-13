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

function(amdinfer_get_example_target target example)
  string(REGEX REPLACE "^${AMDINFER_EXAMPLE_ROOT}" "" BASE_PATH
                       ${CMAKE_CURRENT_SOURCE_DIR}
  )
  string(REGEX REPLACE "\/" "_" BASE_NAME ${BASE_PATH})

  # check for an extra argument to set the prefix. use 'example' by default
  list(LENGTH ARGN extra_count)
  if(extra_count GREATER 0)
    list(GET ARGN 0 prefix)
  elseif(extra_count GREATER 1)
    message(FATAL_ERROR "Cannot pass more than three args to this function")
  else()
    set(prefix "example")
  endif()

  set(${target} ${prefix}${BASE_NAME}-${example} PARENT_SCOPE)
endfunction()

function(amdinfer_add_example example)
  amdinfer_get_example_target(target ${example})

  add_executable(${target} ${example}.cpp)
  target_include_directories(${target} PRIVATE ${AMDINFER_INCLUDE_DIRS})
  target_link_libraries(${target} PRIVATE amdinfer util)
  set_target_options(${target})
endfunction()

function(amdinfer_add_examples examples)
  foreach(example ${examples})
    amdinfer_add_example(${example})
  endforeach()
endfunction()
