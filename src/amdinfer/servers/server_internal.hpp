// Copyright 2022 Xilinx, Inc.
// Copyright 2022 Advanced Micro Devices, Inc.
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

#ifndef GUARD_AMDINFER_SERVERS_SERVER_INTERNAL
#define GUARD_AMDINFER_SERVERS_SERVER_INTERNAL

#include <thread>

#include "amdinfer/build_options.hpp"
#include "amdinfer/core/model_repository.hpp"
#include "amdinfer/core/shared_state.hpp"
#include "amdinfer/servers/server.hpp"

namespace amdinfer {

struct Server::ServerImpl {
#ifdef AMDINFER_ENABLE_HTTP
  bool http_started = false;
  std::thread http_thread;
#endif
#ifdef AMDINFER_ENABLE_GRPC
  bool grpc_started = false;
#endif
  SharedState state;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_SERVERS_SERVER_INTERNAL
