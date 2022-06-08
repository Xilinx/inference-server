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

#include <csignal>              // for signal, SIGINT
#include <cstdlib>              // for exit
#include <cxxopts/cxxopts.hpp>  // for Options, value, OptionAdder
#include <iostream>             // for operator<<, endl, basic_o...
#include <string>               // for string, allocator, operat...

#include "proteus/build_options.hpp"        // for PROTEUS_ENABLE_HTTP, PROT...
#include "proteus/clients/native.hpp"       // for initialize, terminate
#include "proteus/servers/grpc_server.hpp"  // for start, stop
#include "proteus/servers/http_server.hpp"  // for start, stop

/**
 * @brief Handler for incoming interrupt signals
 *
 * @param signum Integer ID for the caught interrupt
 */
void signal_callback_handler(int signum) {
  std::cout << "Caught interrupt " << signum << ". Proteus is ending..."
            << std::endl;
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

  try {
    cxxopts::Options options("proteus-server", "Inference in the cloud");
    // clang-format off
    options.add_options()
    ("model-repository", "Path to the model repository",
      cxxopts::value<std::string>(model_repository))
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
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }
  } catch (const cxxopts::OptionException& e) {
    std::cout << "Error parsing options: " << e.what() << std::endl;
    exit(1);
  }

  proteus::initialize();

#ifdef PROTEUS_ENABLE_GRPC
  std::cout << "gRPC server starting at port " << grpc_port << "\n";
  proteus::grpc::start(grpc_port);
#endif

#ifdef PROTEUS_ENABLE_HTTP
  std::cout << "HTTP server starting at port " << http_port << "\n";
  proteus::http::start(http_port, model_repository);
#else
  while (1) {
    std::this_thread::yield();
  }
#endif

  proteus::terminate();

  return 0;
}
