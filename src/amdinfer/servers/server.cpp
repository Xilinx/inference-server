// Copyright 2022 Xilinx, Inc.
// Copyright 2022 Advanced Micro Devices, Inc.
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

#include "amdinfer/servers/server.hpp"

#include <cstdlib>  // for getenv
#include <string>   // for operator+, string
#include <thread>   // for thread

#include "amdinfer/build_options.hpp"          // for AMDINFER_ENABLE_HTTP
#include "amdinfer/core/manager.hpp"           // for Manager
#include "amdinfer/core/model_repository.hpp"  // for ModelRepository
#include "amdinfer/observation/logging.hpp"    // for initLogger, getLogDirec...
#include "amdinfer/observation/tracing.hpp"    // for startTracer, stopTracer
#include "amdinfer/servers/grpc_server.hpp"    // for start, stop
#include "amdinfer/servers/http_server.hpp"    // for stop, start

#ifdef AMDINFER_ENABLE_AKS
#include <aks/AksSysManagerExt.h>  // for SysManagerExt
#endif

namespace fs = std::filesystem;

namespace amdinfer {

struct Server::ServerImpl {
#ifdef AMDINFER_ENABLE_HTTP
  bool http_started = false;
  std::thread http_thread;
#endif
#ifdef AMDINFER_ENABLE_GRPC
  bool grpc_started = false;
#endif
};

void initializeServerLogging() {
#ifdef AMDINFER_ENABLE_LOGGING
  LogOptions options{
    "server",  // logger_name
    getLogDirectory(),
    true,             // enable file logging
    LogLevel::Trace,  // file log level
    true,             // enable console logging
    LogLevel::Warn    // console log level
  };
  initLogger(options);
#endif
}

void initialize() {
  initializeServerLogging();
#ifdef AMDINFER_ENABLE_TRACING
  startTracer();
#endif

  Manager::getInstance().init();

#ifdef AMDINFER_ENABLE_AKS
  auto* aks_sys_manager = AKS::SysManagerExt::getGlobal();

  // Load all the kernels
  const auto* aks_root = std::getenv("AKS_ROOT");
  if (aks_root == nullptr) {
    throw environment_not_set_error("AKS_ROOT not set");
  }
  const auto kernel_dir = std::string{aks_root} + "/kernel_zoo";
  aks_sys_manager->loadKernels(kernel_dir);
#endif
}

void terminate() {
#ifdef AMDINFER_ENABLE_TRACING
  stopTracer();
#endif
  Manager::getInstance().shutdown();

#ifdef AMDINFER_ENABLE_AKS
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
#ifdef AMDINFER_ENABLE_HTTP
  if (!impl_->http_started) {
    impl_->http_thread = std::thread{http::start, port};
    impl_->http_started = true;
  }
#endif
}

void Server::stopHttp() const {
#ifdef AMDINFER_ENABLE_HTTP
  if (impl_->http_started) {
    http::stop();
    if (impl_->http_thread.joinable()) {
      impl_->http_thread.join();
    }
  }
#endif
}

void Server::startGrpc([[maybe_unused]] uint16_t port) const {
#ifdef AMDINFER_ENABLE_GRPC
  if (!impl_->grpc_started) {
    grpc::start(port);
    impl_->grpc_started = true;
  }
#endif
}

void Server::stopGrpc() const {
#ifdef AMDINFER_ENABLE_GRPC
  if (impl_->grpc_started) {
    grpc::stop();
  }
#endif
}

void Server::setModelRepository(const fs::path& path) {
  ModelRepository::setRepository(path.string());
}

void Server::enableRepositoryMonitoring(bool use_polling) {
  ModelRepository::enableRepositoryMonitoring(use_polling);
}

}  // namespace amdinfer
