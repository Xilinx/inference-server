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

add_library(batcher OBJECT batcher.cpp)
target_include_directories(batcher PRIVATE ${PROTEUS_INCLUDE_DIRS})
target_link_libraries(batcher INTERFACE core observation Threads::Threads)
enable_ipo_on_target(batcher)

add_library(hardBatcher OBJECT hard.cpp)
target_include_directories(hardBatcher PRIVATE ${PROTEUS_INCLUDE_DIRS})
target_link_libraries(hardBatcher INTERFACE batcher core observation)
enable_ipo_on_target(hardBatcher)

add_library(softBatcher OBJECT soft.cpp)
target_include_directories(softBatcher PRIVATE ${PROTEUS_INCLUDE_DIRS})
target_link_libraries(softBatcher INTERFACE batcher core observation)
enable_ipo_on_target(softBatcher)

set(BATCHING_OBJS
  $<TARGET_OBJECTS:batcher>
  $<TARGET_OBJECTS:hardBatcher>
  $<TARGET_OBJECTS:softBatcher>
)

add_library(batching INTERFACE)
target_link_libraries(batching INTERFACE
  ${BATCHING_OBJS}
  batcher
  hardBatcher
  softBatcher
)

set(BATCHING_OBJS ${BATCHING_OBJS} PARENT_SCOPE)
