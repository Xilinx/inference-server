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

set(tests resnet50_fp16)

amdinfer_add_system_tests("${tests}")

foreach(test ${tests})
  amdinfer_get_test_target(target ${test})
  target_link_libraries(
    ${target} PRIVATE opencv_core opencv_imgproc opencv_imgcodecs
  )
endforeach()
