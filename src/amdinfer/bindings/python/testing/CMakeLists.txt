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

add_library(testing_py OBJECT testing.cpp)
target_include_directories(
  testing_py PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/.. ${AMDINFER_INCLUDE_DIRS}
                     ${Python_INCLUDE_DIRS} ${AMDINFER_TEST_INCLUDE_DIRS}
)
target_link_libraries(testing_py PRIVATE ${Python_LIBRARIES} testing)
set_target_options(testing_py)
add_dependencies(testing_py pybind11Mkdoc)

set(TESTING_OBJS testing_py PARENT_SCOPE)
