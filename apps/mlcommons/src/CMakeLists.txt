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

add_executable(mlperf main.cpp query_sample_library.cpp system_under_test.cpp)
target_link_libraries(
  mlperf PRIVATE mlperf_loadgen amdinfer Threads::Threads opencv_core
                 opencv_imgproc opencv_imgcodecs
)
if(NOT PROJECT_IS_TOP_LEVEL)
  set_target_options(mlperf)
  target_include_directories(mlperf PRIVATE ${AMDINFER_PUBLIC_INCLUDE_DIRS})
endif()
