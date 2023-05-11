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

set(base_targets "")
if(${AMDINFER_ENABLE_LOGGING})
  list(APPEND base_targets logging)
endif()
if(${AMDINFER_ENABLE_METRICS})
  list(APPEND base_targets metrics)
endif()
if(${AMDINFER_ENABLE_TRACING})
  list(APPEND base_targets tracing)
endif()
set(derived_targets "")
amdinfer_add_targets(
  targets target_objects "${base_targets}" "${derived_targets}" ""
)

if(${AMDINFER_ENABLE_LOGGING})
  target_link_libraries(logging INTERFACE spdlog::spdlog)
endif()
if(${AMDINFER_ENABLE_METRICS})
  target_link_libraries(metrics INTERFACE prometheus-cpp::core util)
endif()
if(${AMDINFER_ENABLE_TRACING})
  target_link_libraries(
    tracing
    INTERFACE opentelemetry-cpp::trace
              opentelemetry-cpp::resources
              opentelemetry-cpp::ostream_span_exporter
              opentelemetry-cpp::otlp_http_exporter
              opentelemetry-cpp::otlp_http_client
  )
endif()

add_library(observation INTERFACE)
target_link_libraries(observation INTERFACE ${targets} ${target_objects})
