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

include(GNUInstallDirs)

list(APPEND workers
    Echo
    InvertImage
    InvertVideo
    # Fake
)

if(${PROTEUS_ENABLE_VITIS})
    set(workers ${workers} Xmodel)
endif()

foreach( worker ${workers})
    set(worker_name worker${worker})

    # convert name to file name: separate by capital letters, add underscores and lowercase
    string(REGEX MATCHALL "[A-Z][a-z0-9]+" worker_list "${worker}")
    string (REPLACE ";" "_" worker_list_str "${worker_list}")
    string(TOLOWER ${worker_list_str} file_name)

    add_library(${worker_name} SHARED ${file_name}.cpp)
    target_link_options(${worker_name} PUBLIC "LINKER:-E")
    target_link_libraries(${worker_name} PRIVATE batching buffers core)
    target_include_directories(${worker_name} PRIVATE ${PROTEUS_INCLUDE_DIRS})
    enable_ipo_on_target(${worker_name})

    list(APPEND WORKER_TARGETS ${worker_name})
endforeach()

target_link_libraries(workerInvertImage PRIVATE base64 opencv_core opencv_imgcodecs)
target_link_libraries(workerInvertVideo PRIVATE base64 opencv_core opencv_imgcodecs opencv_videoio)
if(${PROTEUS_ENABLE_VITIS})
    target_link_libraries(workerXmodel PRIVATE parse_env vart-runner target-factory xir observation)
endif()
# target_link_libraries(workerFake PRIVATE observation)

if(${PROTEUS_ENABLE_AKS})

    set(aks_workers
        Resnet50
        Resnet50Stream
        AksDetect
        AksDetectStream
    )

    foreach( worker ${aks_workers})
        set(worker_name worker${worker})
        # convert name to file name: separate by capital letters, add underscores and lowercase
        string(REGEX MATCHALL "[A-Z][a-z0-9]+" worker_list "${worker}")
        string (REPLACE ";" "_" worker_list_str "${worker_list}")
        string(TOLOWER ${worker_list_str} file_name)

        add_library(${worker_name} SHARED ${file_name}.cpp)
        target_link_options(${worker_name} PUBLIC "LINKER:-E")
        target_link_libraries(${worker_name} PRIVATE
            batching buffers core base64 parse_env observation
            aks vart-runner xir
            opencv_core opencv_imgcodecs
        )
        target_include_directories(${worker_name} PRIVATE ${PROTEUS_INCLUDE_DIRS})
        enable_ipo_on_target(${worker_name})

        list(APPEND WORKER_TARGETS ${worker_name})
    endforeach()

    target_link_libraries(workerResnet50Stream PRIVATE opencv_imgproc opencv_videoio)
    target_link_libraries(workerAksDetectStream PRIVATE opencv_videoio)

  # AKS Worker
    add_library(workerAks SHARED aks.cpp)
    target_link_options(workerAks PUBLIC "LINKER:-E")
    target_link_libraries(workerAks
        PRIVATE batching buffers core parse_env observation
        PRIVATE aks vart-runner xir
    )
    target_include_directories(workerAks PRIVATE ${PROTEUS_INCLUDE_DIRS})
    enable_ipo_on_target(workerAks)
    list(APPEND WORKER_TARGETS workerAks)
else()
    message(STATUS "AKS not enabled, skipping AKS workers")
endif()

# if(NOT DURATION)
#     set(DURATION 1ms)
#     message(STATUS "Using default DURATION value of " ${DURATION})
# else()
#     message(STATUS "Using DURATION value of " ${DURATION})
# endif()
# target_compile_definitions(workerFake PRIVATE DURATION=${DURATION})
# unset(DURATION CACHE)

install(TARGETS ${WORKER_TARGETS}
    LIBRARY COMPONENT proteus_Runtime
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/proteus"
)
