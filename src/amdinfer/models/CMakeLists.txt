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

set(filenames base64_encode base64_decode echo echo_multi invert_image)
set(targets "")

function(amdinfer_get_target_name target filename)
  set(${target} ${filename} PARENT_SCOPE)
endfunction()

foreach(filename ${filenames})
  amdinfer_get_target_name(target ${filename})
  add_library(${target} SHARED ${filename}.cpp)
  target_include_directories(${target} PRIVATE ${AMDINFER_INCLUDE_DIRS})
  set_target_options(${target})

  list(APPEND targets ${target})
endforeach()

# link these models against OpenCV
foreach(filename base64_encode base64_decode invert_image)
  amdinfer_get_target_name(target ${filename})
  target_link_libraries(${target} PRIVATE opencv_core opencv_imgcodecs)
endforeach()

# create a custom target to create an entry in the repository if all these
# targets succeed
if(NOT AMDINFER_INSTALL)
  add_custom_target(invert_image_repository ALL)
  foreach(filename base64_encode base64_decode invert_image)
    amdinfer_get_target_name(target ${filename})
    add_dependencies(invert_image_repository ${target})
  endforeach()

  add_custom_command(
    TARGET invert_image_repository
    POST_BUILD
    COMMENT
      "Copying base64_encode, base64_decode and invert_image to invert_image model in repository"
    COMMAND mkdir -p
            ${PROJECT_SOURCE_DIR}/external/artifacts/repository/invert_image/1
    COMMAND
      cp libbase64_decode.so
      ${PROJECT_SOURCE_DIR}/external/artifacts/repository/invert_image/1/base64_decode.so
    COMMAND
      cp libinvert_image.so
      ${PROJECT_SOURCE_DIR}/external/artifacts/repository/invert_image/1/invert_image.so
    COMMAND
      cp libbase64_encode.so
      ${PROJECT_SOURCE_DIR}/external/artifacts/repository/invert_image/1/base64_encode.so
    COMMAND
      cp ${PROJECT_SOURCE_DIR}/external/repository_metadata/invert_image.toml
      ${PROJECT_SOURCE_DIR}/external/artifacts/repository/invert_image/invert_image.toml
  )
endif()

if(NOT SKBUILD)
  install(TARGETS ${targets} LIBRARY COMPONENT amdinfer_Runtime
          LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  )
endif()
