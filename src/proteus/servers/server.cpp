// Copyright 2022 Xilinx Inc.
// Copyright 2022 Advanced Micro Devices Inc.
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

#include "proteus/servers/server.hpp"

#include <cstdlib>  // for getenv
#include <string>   // for operator+, string
#include <thread>   // for thread

#include "proteus/build_options.hpp"          // for PROTEUS_ENABLE_HTTP
#include "proteus/core/manager.hpp"           // for Manager
#include "proteus/core/model_repository.hpp"  // for ModelRepository
#include "proteus/observation/logging.hpp"    // for initLogger, getLogDirec...
#include "proteus/observation/tracing.hpp"    // for startTracer, stopTracer
#include "proteus/servers/grpc_server.hpp"    // for start, stop
#include "proteus/servers/http_server.hpp"    // for stop, start

#ifdef PROTEUS_ENABLE_AKS
#include <aks/AksSysManagerExt.h>  // for SysManagerExt
#endif

namespace proteus {

class Server::ServerImpl {
 public:
#ifdef PROTEUS_ENABLE_HTTP
  bool http_started_ = false;
  std::thread http_thread_;
#endif
#ifdef PROTEUS_ENABLE_GRPC
  bool grpc_started_ = false;
#endif
};

void initializeServerLogging() {
#ifdef PROTEUS_ENABLE_LOGGING
  LogOptions options{
    "server",  // logger_name
    getLogDirectory(),
    true,              // enable file logging
    LogLevel::kTrace,  // file log level
    true,              // enable console logging
    LogLevel::kWarn    // console log level
  };
  initLogger(options);
#endif
}

void initialize() {
  initializeServerLogging();
#ifdef PROTEUS_ENABLE_TRACING
  startTracer();
#endif

  Manager::getInstance().init();

  ModelRepository::setRepository(std::string(std::getenv("PROTEUS_ROOT")) +
                                 "/external/repository");

#ifdef PROTEUS_ENABLE_AKS
  auto* aks_sys_manager = AKS::SysManagerExt::getGlobal();

  // Load all the kernels
  aks_sys_manager->loadKernels(std::string(std::getenv("AKS_ROOT")) +
                               std::string("/kernel_zoo"));
#endif
}

void terminate() {
#ifdef PROTEUS_ENABLE_TRACING
  stopTracer();
#endif
  Manager::getInstance().shutdown();

#ifdef PROTEUS_ENABLE_AKS
  AKS::SysManagerExt::deleteGlobal();
#endif
}

Server::Server() {
  this->impl_ = std::make_unique<Server::ServerImpl>();
  initialize();
}

Server::~Server() {
  stopHttp();
  stopGrpc();
  terminate();
}

void Server::startHttp([[maybe_unused]] uint16_t port) const {
#ifdef PROTEUS_ENABLE_HTTP
  if (!impl_->http_started_) {
    impl_->http_thread_ = std::thread{http::start, port};
    impl_->http_started_ = true;
  }
#endif
}

void Server::stopHttp() const {
#ifdef PROTEUS_ENABLE_HTTP
  if (impl_->http_started_) {
    http::stop();
    if (impl_->http_thread_.joinable()) {
      impl_->http_thread_.join();
    }
  }
#endif
}

void Server::startGrpc([[maybe_unused]] uint16_t port) const {
#ifdef PROTEUS_ENABLE_GRPC
  if (!impl_->grpc_started_) {
    grpc::start(port);
    impl_->grpc_started_ = true;
  }
#endif
}

void Server::stopGrpc() const {
#ifdef PROTEUS_ENABLE_GRPC
  if (impl_->grpc_started_) {
    grpc::stop();
  }
#endif
}

}  // namespace proteus
