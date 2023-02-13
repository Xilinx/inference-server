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

set(OBSERVATION_OBJS "")
set(targets "")

if(${AMDINFER_ENABLE_LOGGING})
  add_library(logging OBJECT logging.cpp)
  target_link_libraries(logging INTERFACE spdlog)
  target_include_directories(logging PRIVATE ${AMDINFER_INCLUDE_DIRS})
  set_target_options(logging)

  list(APPEND targets logging)

  set(OBSERVATION_OBJS ${OBSERVATION_OBJS} $<TARGET_OBJECTS:logging>)
endif()

if(${AMDINFER_ENABLE_METRICS})
  add_library(metrics OBJECT metrics.cpp)
  target_link_libraries(metrics INTERFACE prometheus-cpp-core util)
  target_include_directories(metrics PRIVATE ${AMDINFER_INCLUDE_DIRS})
  set_target_options(metrics)

  list(APPEND targets metrics)

  set(OBSERVATION_OBJS ${OBSERVATION_OBJS} $<TARGET_OBJECTS:metrics>)
endif()

if(${AMDINFER_ENABLE_TRACING})
  add_library(tracing OBJECT tracing.cpp)
  target_include_directories(tracing PRIVATE ${AMDINFER_INCLUDE_DIRS})
  # switch opentelemetry_exporter_jaeger_trace to
  # opentelemetry_exporter_ostream_span for debugging traces locally
  target_link_libraries(
    tracing INTERFACE opentelemetry_trace libopentelemetry_resources.so
                      opentelemetry_exporter_jaeger_trace
  )
  set_target_options(tracing)

  list(APPEND targets tracing)

  set(OBSERVATION_OBJS ${OBSERVATION_OBJS} $<TARGET_OBJECTS:tracing>)
endif()

add_library(observation STATIC ${OBSERVATION_OBJS})
target_link_libraries(observation INTERFACE ${targets})
set_target_options(observation)

set(OBSERVATION_OBJS ${OBSERVATION_OBJS} PARENT_SCOPE)
