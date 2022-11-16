..
    Copyright 2021 Xilinx, Inc.
    Copyright 2022, Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

.. _logs:

Logs
====

Logs collected by AMD Inference Server are placed in ``~/.amdinfer/logs/`` by default.

AMD Inference Server Logs
-------------------------

Logging in AMD Inference Server is configured in ``amdinfer/observation/logging.*``.
There are multiple knobs that can be tweaked to affect how and which log messages are captured.

.. code-block:: c++

    // in logging.hpp
    #define SPDLOG_ACTIVE_LEVEL XXX

    /*
    Logging must be globally configured by setting SPDLOG_ACTIVE_LEVEL to one of:
    - SPDLOG_LEVEL_TRACE
    - SPDLOG_LEVEL_DEBUG
    - SPDLOG_LEVEL_INFO
    - SPDLOG_LEVEL_WARN
    - SPDLOG_LEVEL_ERROR
    - SPDLOG_LEVEL_CRITICAL
    - SPDLOG_LEVEL_OFF

    This setting configures at compile-time the minimum logging level that's allowed.
    Setting this to OFF removes all logging statements.
    */

    // in logging.cpp
    sink->set_level(...)
    logger->set_level(...)
    logger->flush_on(...)

    /*
    Different kinds of sinks (e.g. file, console) and loggers can have individual
    run-time minimum log level settings which must be one of:
    - spdlog::level::trace
    - spdlog::level::debug
    - spdlog::level::info
    - spdlog::level::warn
    - spdlog::level::error
    - spdlog::level::critical
    - spdlog::level::off
    */


Drogon Logs
-----------

Logging in Drogon is configured in `amdinfer/servers/http_server.cpp` by setting the appropriate log level when initializing the Drogon app.

.. code-block:: c++

    // in http_server.cpp
    drogon::app()
    ...
    .setLogLevel(XXX)
    ...

    /*
    The log level must be one of:
    - trantor::Logger::kTrace
    - trantor::Logger::kDebug
    - trantor::Logger::kInfo
    - trantor::Logger::kWarn
    - trantor::Logger::kError
    - trantor::Logger::kFatal
    */
