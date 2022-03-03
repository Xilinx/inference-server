# Copyright 2022 Xilinx Inc.
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
    server_live
    model_infer
    model_load
)

foreach(target ${targets})
    add_executable(grpc_${target} ${target}.cpp)

    target_include_directories(grpc_${target} PRIVATE ${PROTEUS_PUBLIC_INCLUDE_DIRS})
    target_link_libraries(grpc_${target} PRIVATE proteus gtest gtest_main)
endforeach()
