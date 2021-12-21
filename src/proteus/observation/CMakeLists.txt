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

add_library(observation INTERFACE)

if(${PROTEUS_ENABLE_LOGGING})
    add_library(logging OBJECT logging.cpp)
    target_link_libraries(logging INTERFACE spdlog)
    target_include_directories(logging PRIVATE ${PROTEUS_INCLUDE_DIRS})
    enable_ipo_on_target(logging)

    target_link_libraries(observation INTERFACE
        $<TARGET_OBJECTS:logging> logging
    )

    set(OBSERVATION_OBJS
        $<TARGET_OBJECTS:logging>
    )
endif()

if(${PROTEUS_ENABLE_METRICS})
    add_library(metrics OBJECT metrics.cpp)
    target_link_libraries(metrics INTERFACE prometheus-cpp-core)
    target_include_directories(metrics PRIVATE ${PROTEUS_INCLUDE_DIRS})
    enable_ipo_on_target(metrics)

    target_link_libraries(observation INTERFACE
        $<TARGET_OBJECTS:metrics> metrics
    )

    set(OBSERVATION_OBJS
        ${OBSERVATION_OBJS}
        $<TARGET_OBJECTS:metrics>
    )
endif()

if(${PROTEUS_ENABLE_TRACING})
    add_library(tracing OBJECT tracing.cpp)
    target_include_directories(tracing PRIVATE ${PROTEUS_INCLUDE_DIRS})
    # switch opentelemetry_exporter_jaeger_trace to opentelemetry_exporter_ostream_span for debugging traces locally
    target_link_libraries(tracing INTERFACE opentelemetry_trace libopentelemetry_resources.so opentelemetry_exporter_jaeger_trace)
    enable_ipo_on_target(tracing)

    target_link_libraries(observation INTERFACE
        $<TARGET_OBJECTS:tracing> tracing
    )

    set(OBSERVATION_OBJS
        ${OBSERVATION_OBJS}
        $<TARGET_OBJECTS:tracing>
    )
endif()

set(OBSERVATION_OBJS ${OBSERVATION_OBJS} PARENT_SCOPE)
