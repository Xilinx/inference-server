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

set(examples "")

if(${AMDINFER_ENABLE_VITIS})
  if(${AMDINFER_ENABLE_HTTP})
    list(APPEND examples vitis)
  endif()
endif()

if(${AMDINFER_ENABLE_PTZENDNN})
  list(APPEND examples ptzendnn)
endif()

if(${AMDINFER_ENABLE_TFZENDNN})
  if(${AMDINFER_ENABLE_GRPC})
    list(APPEND examples tfzendnn)
  endif()
endif()

if(${AMDINFER_ENABLE_MIGRAPHX})
  if(${AMDINFER_ENABLE_HTTP})
    list(APPEND examples migraphx)
  endif()
endif()

foreach(example ${examples})
  amdinfer_add_example(${example})
  amdinfer_get_example_target(target ${example})
  target_link_libraries(
    ${target} PRIVATE opencv_imgcodecs opencv_imgproc opencv_core
  )
endforeach()
