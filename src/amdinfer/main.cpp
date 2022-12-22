// Copyright 2021 Xilinx, Inc.
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

/**
 * @file
 * @brief Parses the command-line arguments and implements the entrypoint for
 * the amdinfer-server executable
 */

#include <csignal>              // for signal, SIGINT, SIGTERM
#include <cstdlib>              // for exit
#include <cxxopts/cxxopts.hpp>  // for value, OptionAdder, Opt...
#include <efsw/efsw.hpp>        // for FileWatcher
#include <filesystem>           // for path, directory_iterator
#include <iostream>             // for operator<<, basic_ostre...
#include <memory>               // for unique_ptr, make_unique
#include <string>               // for string, allocator, char...

#include "amdinfer/build_options.hpp"          // for AMDINFER_ENABLE_HTTP
#include "amdinfer/clients/native.hpp"         // for NativeClient
#include "amdinfer/core/exceptions.hpp"        // for runtime_error
#include "amdinfer/core/model_repository.hpp"  // for UpdateListener, ModelRe...
#include "amdinfer/observation/logging.hpp"    // for Logger, AMDINFER_LOG_INFO
#include "amdinfer/servers/http_server.hpp"    // for stop
#include "amdinfer/servers/server.hpp"         // for Server

/**
 * @brief Handler for incoming interrupt signals
 *
 * @param signum Integer ID for the caught interrupt
 */
void signalCallbackHandler(int signum) {
  std::cout << "Caught interrupt " << signum
            << ". amdinfer-server is ending...\n";
#ifdef AMDINFER_ENABLE_HTTP
  amdinfer::http::stop();
#endif
}

/**
 * @brief Parses command line options and starts amdinfer-server
 *
 * @param argc Number of command line arguments
 * @param argv Command line arguments
 * @return int Return value at termination
 */
int main(int argc, char* argv[]) {
  signal(SIGINT, signalCallbackHandler);
  signal(SIGTERM, signalCallbackHandler);
#ifdef AMDINFER_ENABLE_HTTP
  uint16_t http_port = kDefaultHttpPort;
#endif
#ifdef AMDINFER_ENABLE_GRPC
  uint16_t grpc_port = kDefaultGrpcPort;
#endif
  std::string model_repository = "/mnt/models";
  bool enable_repository_watcher = false;
  bool use_polling_watcher = false;

  try {
    cxxopts::Options options("amdinfer-server", "Inference in the cloud");
    // clang-format off
    options.add_options()
    ("model-repository", "Path to the model repository",
      cxxopts::value(model_repository))
    ("enable-repository-watcher",
      "Actively monitor the model-repository directory for new models",
      cxxopts::value(enable_repository_watcher))
    ("use-polling-watcher", "Use polling to monitor model-repository directory",
      cxxopts::value(use_polling_watcher))
#ifdef AMDINFER_ENABLE_HTTP
    ("http-port", "Port to use for HTTP server", cxxopts::value(http_port))
#endif
#ifdef AMDINFER_ENABLE_GRPC
    ("grpc-port", "Port to use for gRPC server", cxxopts::value(grpc_port))
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

  amdinfer::Server server;

  amdinfer::Logger logger{amdinfer::Loggers::kServer};

  server.setModelRepository(model_repository);
  AMDINFER_LOG_INFO(logger, "Using model repository: " + model_repository);

  if (enable_repository_watcher) {
    server.enableRepositoryMonitoring(use_polling_watcher);
  }

#ifdef AMDINFER_ENABLE_GRPC
  std::cout << "gRPC server starting at port " << grpc_port << "\n";
  server.startGrpc(grpc_port);
#endif

#ifdef AMDINFER_ENABLE_HTTP
  std::cout << "HTTP server starting at port " << http_port << std::endl;
  server.startHttp(http_port);
#else
  while (1) {
    std::this_thread::yield();
  }
#endif

  return 0;
}
