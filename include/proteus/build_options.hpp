// Copyright 2021 Xilinx Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file
 * @brief Defines the build information. This file is updated
 * automatically by CMake. To update, recompile the source code.
 */

#ifndef GUARD_AMDINFER_BUILD_OPTIONS_HPP
#define GUARD_AMDINFER_BUILD_OPTIONS_HPP

/// Enables testing
#define AMDINFER_BUILD_TESTING
/// Enables rest endpoints for inference
#define AMDINFER_ENABLE_REST
/// Enables metric collection
#define AMDINFER_ENABLE_METRICS
/// Enables HTTP server
#define AMDINFER_ENABLE_HTTP
/// Enables gRPC server
#define AMDINFER_ENABLE_GRPC
/// Enables tracing
#define AMDINFER_ENABLE_TRACING
/// Enables logging
#define AMDINFER_ENABLE_LOGGING
/// Enables AKS
#define AMDINFER_ENABLE_AKS
/// Enables Vitis
#define AMDINFER_ENABLE_VITIS
/// Enables TF+ZenDNN
/* #undef AMDINFER_ENABLE_TFZENDNN */
/// Enables PT+ZenDNN
/* #undef AMDINFER_ENABLE_PTZENDNN */
/// Enables MIGraphX
/* #undef AMDINFER_ENABLE_MIGRAPHX */

/// Port used by the HTTP server by default
constexpr auto kDefaultHttpPort = 8998;

/// Port used by the gRPC server by default
constexpr auto kDefaultGrpcPort = 50051;

/// Number of threads used by Drogon
constexpr auto kDefaultDrogonThreads = 16;

/// Maximum size of a HTTP request body in MiB. Arbitrarily set to 400MiB
constexpr auto kMaxClientBodySize = 419430400;

/// Maximum size of gRPC messages in bytes. Arbitrarily set to 20MiB
constexpr auto kMaxGrpcMessageSize = 20971520;

/// Maximum number of characters usable for a model name used in an endpoint.
constexpr auto kMaxModelNameSize = 64;
#endif  // GUARD_AMDINFER_BUILD_OPTIONS_HPP
