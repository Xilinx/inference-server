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

add_library(pre_post_py OBJECT pre_post.cpp)
target_include_directories(
  pre_post_py PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/.. ${AMDINFER_INCLUDE_DIRS}
                      ${Python_INCLUDE_DIRS}
)
target_link_libraries(
  pre_post_py PRIVATE ${Python_LIBRARIES} opencv_core opencv_imgproc
                      opencv_imgcodecs
)
set_target_options(pre_post_py)
add_dependencies(pre_post_py pybind11Mkdoc)

set(PRE_POST_OBJS pre_post_py PARENT_SCOPE)
