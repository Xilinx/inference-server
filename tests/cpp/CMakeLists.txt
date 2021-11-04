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

list(APPEND directories
    native
)

list(APPEND libraries)
list(APPEND filenames)
foreach(directory ${directories})
    add_subdirectory(${directory})

    foreach(target ${TARGETS})
        set(filenames ${filenames} ${directory}/${target}_test.cpp)
        get_target_property(TARGET_LIBS ${target} LINK_LIBRARIES)
        set(libraries ${libraries} ${TARGET_LIBS})
    endforeach()
endforeach()

add_executable(test_gtest gtest.cpp ${filenames})

target_link_libraries(test_gtest PRIVATE gtest proteus ${libraries})
target_include_directories(test_gtest PRIVATE ${PROTEUS_INCLUDE_DIRS})

include(GoogleTest)
gtest_discover_tests(test_gtest)
