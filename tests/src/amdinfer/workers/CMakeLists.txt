# Copyright 2023 Advanced Micro Devices, Inc.
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

include(GNUInstallDirs)

set(workers Fake)

function(amdinfer_get_worker_target target filename worker)
  # convert name to file name: separate by capital letters, add underscores and
  # lowercase
  string(REGEX MATCHALL "[A-Z][a-z0-9]*" worker_list "${worker}")
  string(REPLACE ";" "_" worker_list_str "${worker_list}")
  string(TOLOWER ${worker_list_str} file_name)

  # convert name to library name: capitalize first letter
  string(TOLOWER ${worker} worker_lower)
  string(SUBSTRING ${worker_lower} 0 1 first_letter)
  string(TOUPPER ${first_letter} first_letter)
  string(REGEX REPLACE "^.(.*)" "${first_letter}\\1" worker_pascal
                       "${worker_lower}"
  )
  set(${target} worker${worker_pascal} PARENT_SCOPE)
  set(${filename} ${file_name} PARENT_SCOPE)
endfunction()

foreach(worker ${workers})
  amdinfer_get_worker_target(target filename ${worker})

  add_library(${target} SHARED ${filename}.cpp)
  target_include_directories(${target} PRIVATE ${AMDINFER_INCLUDE_DIRS})
  set_target_options(${target})
endforeach()
