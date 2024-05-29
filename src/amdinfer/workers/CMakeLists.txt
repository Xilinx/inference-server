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

include(GNUInstallDirs)

set(workers InvertVideo CPlusPlus Responder)

if(${AMDINFER_ENABLE_VITIS})
  list(APPEND workers Xmodel)
endif()

if(${AMDINFER_ENABLE_TFZENDNN})
  list(APPEND workers TfZendnn)
endif()

if(${AMDINFER_ENABLE_PTZENDNN})
  list(APPEND workers PtZendnn)
endif()

if(${AMDINFER_ENABLE_MIGRAPHX})
  list(APPEND workers Migraphx)
endif()

if(${AMDINFER_ENABLE_ROCAL})
  list(APPEND workers Rocal)
endif()

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

  list(APPEND WORKER_TARGETS ${target})
endforeach()

target_link_libraries(
  workerInvertvideo PRIVATE base64 opencv_core opencv_imgcodecs opencv_videoio
)
if(${AMDINFER_ENABLE_VITIS})
  target_link_libraries(
    workerXmodel PRIVATE vart::runner target-factory::target-factory xir::xir
                         observation util
  )
endif()

if(${AMDINFER_ENABLE_MIGRAPHX})
  target_link_libraries(
    workerMigraphx PRIVATE migraphx::c opencv_imgcodecs opencv_imgproc
                           opencv_core
  )
endif()

if(${AMDINFER_ENABLE_ROCAL})
  #TODO : Use find_package(rocal) instead of using the full path
  find_library(ROCAL_LIBRARY NAMES rocal PATHS /opt/vcpkg/x64-linux-dynamic/lib NO_DEFAULT_PATH)
  target_link_libraries(
    workerRocal PRIVATE ${ROCAL_LIBRARY} opencv_imgcodecs opencv_imgproc
                           opencv_core
  )
endif()

if(${AMDINFER_ENABLE_TFZENDNN})
  target_include_directories(
    workerTfzendnn SYSTEM BEFORE PRIVATE /usr/include/tfzendnn
  )
  target_link_libraries(
    # not linking to tensorflow libraries (see the tfzendnn worker)
    workerTfzendnn PRIVATE ${CMAKE_DL_LIBS}
  )
endif()

if(${AMDINFER_ENABLE_PTZENDNN})
  target_include_directories(
    workerPtzendnn SYSTEM PRIVATE /usr/include/ptzendnn
  )
  target_link_libraries(workerPtzendnn PRIVATE torch torch_cpu c10)
endif()

if(${AMDINFER_ENABLE_AKS})

  set(aks_workers Resnet50 Resnet50Stream AksDetect AksDetectStream)

  foreach(worker ${aks_workers})
    amdinfer_get_worker_target(target filename ${worker})

    add_library(${target} SHARED ${filename}.cpp)
    target_link_options(${target} PUBLIC "LINKER:-E")
    target_link_libraries(
      ${target} PRIVATE aks::aks vart::runner xir::xir opencv_core
                        opencv_imgcodecs
    )
    target_include_directories(${target} PRIVATE ${AMDINFER_INCLUDE_DIRS})
    # add_dependencies(${target} aks-kernels)
    set_target_options(${target})

    list(APPEND WORKER_TARGETS ${target})
  endforeach()

  target_link_libraries(
    workerResnet50stream PRIVATE opencv_imgproc opencv_videoio
  )
  target_link_libraries(workerAksdetectstream PRIVATE opencv_videoio)
else()
  message(STATUS "AKS not enabled, skipping AKS workers")
endif()

# if(NOT DURATION) set(DURATION 1ms) message(STATUS "Using default DURATION
# value of " ${DURATION}) else() message(STATUS "Using DURATION value of "
# ${DURATION}) endif() target_compile_definitions(workerFake PRIVATE
# DURATION=${DURATION}) unset(DURATION CACHE)

if(NOT SKBUILD)
  install(TARGETS ${WORKER_TARGETS} LIBRARY COMPONENT amdinfer_Runtime
          LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  )
endif()
