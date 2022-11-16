// Copyright 2022 Xilinx Inc.
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
 * @brief Defines the gRPC server in Proteus
 */

#ifndef GUARD_AMDINFER_SERVERS_GRPC_SERVER
#define GUARD_AMDINFER_SERVERS_GRPC_SERVER

#include "amdinfer/build_options.hpp"

#ifdef AMDINFER_ENABLE_GRPC

namespace amdinfer::grpc {

void start(int port);
void stop();

}  // namespace amdinfer::grpc

#endif  // AMDINFER_ENABLE_GRPC

#endif  // GUARD_AMDINFER_SERVERS_GRPC_SERVER
