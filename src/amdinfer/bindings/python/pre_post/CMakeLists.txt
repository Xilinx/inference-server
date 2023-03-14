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

set(derived_targets image_preprocess)

amdinfer_add_targets(targets target_objects "" "${derived_targets}" _py)

foreach(target ${derived_targets})
  target_include_directories(
    ${target}_py PRIVATE ${AMDINFER_PYTHON_INCLUDE_DIRS}
  )
endforeach()

add_library(pre_post_py INTERFACE)
target_link_libraries(
  pre_post_py INTERFACE ${targets} ${target_objects} opencv_core opencv_imgproc
                        opencv_imgcodecs
)
add_dependencies(pre_post_py pybind11Mkdoc)
