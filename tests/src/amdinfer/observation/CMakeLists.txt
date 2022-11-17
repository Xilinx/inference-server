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

add_library(fake_observation INTERFACE)

if(${AMDINFER_ENABLE_LOGGING})
  # add_library(fake_logging OBJECT logging.cpp) # target_link_libraries(logging
  # INTERFACE spdlog) target_include_directories(fake_logging BEFORE PRIVATE
  # ${AMDINFER_TEST_INCLUDE_DIRS}) target_include_directories(fake_logging AFTER
  # PRIVATE ${AMDINFER_INCLUDE_DIRS}) enable_ipo_on_target(fake_logging)

  target_link_libraries(
    fake_observation INTERFACE $<TARGET_OBJECTS:logging> logging
  )
endif()

if(${AMDINFER_ENABLE_METRICS})
  target_link_libraries(
    fake_observation INTERFACE $<TARGET_OBJECTS:metrics> metrics
  )
endif()

if(${AMDINFER_ENABLE_TRACING})
  add_library(fake_tracing OBJECT tracing.cpp)
  target_include_directories(
    fake_tracing BEFORE PRIVATE ${AMDINFER_TEST_INCLUDE_DIRS}
  )
  target_include_directories(
    fake_tracing AFTER PRIVATE ${AMDINFER_INCLUDE_DIRS}
  )
  # switch opentelemetry_exporter_jaeger_trace to
  # opentelemetry_exporter_ostream_span for debugging traces locally
  # target_link_libraries(tracing INTERFACE opentelemetry_trace
  # libopentelemetry_resources.so opentelemetry_exporter_jaeger_trace)
  enable_ipo_on_target(fake_tracing)

  target_link_libraries(
    fake_observation INTERFACE $<TARGET_OBJECTS:fake_tracing> fake_tracing
  )
endif()
