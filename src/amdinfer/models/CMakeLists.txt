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

set(filenames echo)
set(targets "")

function(amdinfer_get_target_name target filename)
  set(${target} ${filename}_model PARENT_SCOPE)
endfunction()

foreach(filename ${filenames})
  amdinfer_get_target_name(target ${filename})
  add_library(${target} SHARED ${filename}.cpp)
  target_include_directories(${target} PRIVATE ${AMDINFER_INCLUDE_DIRS})
  set_target_options(${target})

  list(APPEND targets ${target})
endforeach()

if(NOT SKBUILD)
  install(TARGETS ${targets} LIBRARY COMPONENT amdinfer_Runtime
          LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  )
endif()
