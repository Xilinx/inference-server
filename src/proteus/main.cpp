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
 * @brief Parses the command-line arguments and implements Proteus's entrypoint
 */

#include <csignal>              // for signal, SIGINT, SIGTERM
#include <cstdlib>              // for exit
#include <cxxopts/cxxopts.hpp>  // for value, OptionAdder, Opt...
#include <efsw/efsw.hpp>        // for FileWatcher
#include <filesystem>           // for path, directory_iterator
#include <iostream>             // for operator<<, basic_ostre...
#include <memory>               // for unique_ptr, make_unique
#include <string>               // for string, allocator, char...

#include "proteus/build_options.hpp"          // for PROTEUS_ENABLE_HTTP
#include "proteus/clients/native.hpp"         // for NativeClient
#include "proteus/core/exceptions.hpp"        // for runtime_error
#include "proteus/core/model_repository.hpp"  // for UpdateListener, ModelRe...
#include "proteus/observation/logging.hpp"    // for Logger, PROTEUS_LOG_INFO
#include "proteus/servers/http_server.hpp"    // for stop
#include "proteus/servers/server.hpp"         // for Server

namespace fs = std::filesystem;

/**
 * @brief Handler for incoming interrupt signals
 *
 * @param signum Integer ID for the caught interrupt
 */
void signal_callback_handler(int signum) {
  std::cout << "Caught interrupt " << signum << ". Proteus is ending...\n";
#ifdef PROTEUS_ENABLE_HTTP
  proteus::http::stop();
#endif
}

/**
 * @brief Parses command line options and starts Proteus
 *
 * @param argc Number of command line arguments
 * @param argv Command line arguments
 * @return int Return value at termination
 */
int main(int argc, char* argv[]) {
  signal(SIGINT, signal_callback_handler);
  signal(SIGTERM, signal_callback_handler);
#ifdef PROTEUS_ENABLE_HTTP
  int http_port = kDefaultHttpPort;
#endif
#ifdef PROTEUS_ENABLE_GRPC
  int grpc_port = kDefaultGrpcPort;
#endif
  std::string model_repository = "/mnt/models";
  bool enable_repository_watcher = false;
  bool use_polling_watcher = false;

  try {
    cxxopts::Options options("proteus-server", "Inference in the cloud");
    // clang-format off
    options.add_options()
    ("model-repository", "Path to the model repository",
      cxxopts::value<std::string>(model_repository))
    ("enable-repository-watcher",
      "Actively monitor the model-repository directory for new models",
      cxxopts::value<bool>(enable_repository_watcher))
    ("use-polling-watcher", "Use polling to monitor model-repository directory",
      cxxopts::value<bool>(use_polling_watcher))
#ifdef PROTEUS_ENABLE_HTTP
    ("http-port", "Port to use for HTTP server", cxxopts::value<int>(http_port))
#endif
#ifdef PROTEUS_ENABLE_GRPC
    ("grpc-port", "Port to use for gRPC server", cxxopts::value<int>(grpc_port))
#endif
    ("help", "Print help");
    // clang-format on

    auto result = options.parse(argc, argv);

    if (result.count("help") != 0U) {
      std::cout << options.help({""}) << "\n";
      exit(0);
    }
  } catch (const cxxopts::OptionException& e) {
    std::cout << "Error parsing options: " << e.what() << "\n";
    exit(1);
  }

  proteus::Server server;

  proteus::Logger logger{proteus::Loggers::kServer};

  server.setModelRepository(model_repository);
  PROTEUS_LOG_INFO(logger, "Using model repository: " + model_repository);

  if (enable_repository_watcher) {
    server.enableRepositoryMonitoring(use_polling_watcher);
  }

#ifdef PROTEUS_ENABLE_GRPC
  std::cout << "gRPC server starting at port " << grpc_port << "\n";
  server.startGrpc(grpc_port);
#endif

#ifdef PROTEUS_ENABLE_HTTP
  std::cout << "HTTP server starting at port " << http_port << std::endl;
  server.startHttp(http_port);
#else
  while (1) {
    std::this_thread::yield();
  }
#endif

  return 0;
}
