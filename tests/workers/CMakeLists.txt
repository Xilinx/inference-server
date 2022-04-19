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

list(APPEND targets
    echo
)

if(${PROTEUS_ENABLE_VITIS})
    set(targets ${targets} xmodel)
endif()

if(${PROTEUS_ENABLE_AKS})
    set(targets ${targets} facedetect)
endif()

foreach(target ${targets})
    add_executable(test_${target} test_${target}.cpp)
    target_link_libraries(test_${target} PRIVATE proteus Threads::Threads gtest gtest_main)
    target_include_directories(test_${target} PRIVATE ${PROTEUS_PUBLIC_INCLUDE_DIRS})

    gtest_discover_tests(test_${target})
endforeach()

if(${PROTEUS_ENABLE_VITIS})
    target_link_libraries(test_xmodel PRIVATE
        xir vart-runner
    )
endif()

if(${PROTEUS_ENABLE_AKS})
    target_link_libraries(test_facedetect PRIVATE
        opencv_core opencv_imgcodecs stdc++fs
    )
endif()

foreach(target ${targets})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${target}.cpp)
        add_executable(${target} ${target}.cpp)
        target_link_libraries(${target} PRIVATE $<TARGET_PROPERTY:test_${target},LINK_LIBRARIES>)
        target_include_directories(${target} PRIVATE ${PROTEUS_PUBLIC_INCLUDE_DIRS})
    endif()
endforeach()
