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

  try {
    cxxopts::Options options("Proteus Server",
                             "Inference in the cloud with Xilinx FPGAs");
    options.allow_unrecognised_options().add_options()
#ifdef PROTEUS_ENABLE_HTTP
      ("http_port", "Port to use for HTTP server",
       cxxopts::value<int>(http_port))
#endif
        ("help", "Print help");

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

#ifdef PROTEUS_ENABLE_HTTP
  std::cout << "Server starting at port " << http_port << "\n";
  proteus::http::start(http_port);
#endif

  proteus::terminate();

  return 0;
}
