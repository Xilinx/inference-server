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

add_executable(grpc_server_live server_live.cpp)

get_target_property(grpc_include lib_grpc INCLUDE_DIRECTORIES)
target_include_directories(grpc_server_live PRIVATE ${grpc_include})
target_link_libraries(grpc_server_live PRIVATE lib_grpc)

add_executable(grpc_model_infer model_infer.cpp)

target_include_directories(grpc_model_infer PRIVATE ${grpc_include})
target_link_libraries(grpc_model_infer PRIVATE lib_grpc)
