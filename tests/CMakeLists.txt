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

set(AMDINFER_TEST_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

include(AddTest)

add_subdirectory(api)
add_subdirectory(models)
add_subdirectory(performance)
add_subdirectory(src/amdinfer)
add_subdirectory(unit)
add_subdirectory(workers)
