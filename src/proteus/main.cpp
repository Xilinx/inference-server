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
#include <efsw/efsw.hpp>        // for FileWatcher
#include <filesystem>
#include <iostream>  // for operator<<, endl, basic_o...
#include <string>    // for string, allocator, operat...

#include "proteus/build_options.hpp"   // for PROTEUS_ENABLE_HTTP, PROT...
#include "proteus/clients/native.hpp"  // for initialize, terminate
#include "proteus/core/model_repository.hpp"  // for ModelRepository
#include "proteus/observation/logging.hpp"    // for logger
#include "proteus/servers/grpc_server.hpp"    // for start, stop
#include "proteus/servers/http_server.hpp"    // for start, stop

namespace fs = std::filesystem;

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
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }
  } catch (const cxxopts::OptionException& e) {
    std::cout << "Error parsing options: " << e.what() << std::endl;
    exit(1);
  }

  proteus::initialize();
  proteus::Logger logger{proteus::Loggers::kServer};

  proteus::ModelRepository::setRepository(model_repository);
  PROTEUS_LOG_INFO(logger, "Using model repository: " + model_repository);

  std::unique_ptr<efsw::FileWatcher> file_watcher;
  std::unique_ptr<proteus::UpdateListener> listener;
  if (enable_repository_watcher) {
    file_watcher = std::make_unique<efsw::FileWatcher>(use_polling_watcher);
    listener = std::make_unique<proteus::UpdateListener>();

    file_watcher->addWatch(model_repository, listener.get(), true);
    file_watcher->watch();

    proteus::NativeClient client;
    for (const auto& path : fs::directory_iterator(model_repository)) {
      if (path.is_directory()) {
        auto model = path.path().filename();
        try {
          client.modelLoad(model, nullptr);
        } catch (const std::runtime_error& e) {
          PROTEUS_LOG_INFO(logger, "Error loading " + model.string());
        }
      }
    }
  }

#ifdef PROTEUS_ENABLE_GRPC
  std::cout << "gRPC server starting at port " << grpc_port << "\n";
  proteus::grpc::start(grpc_port);
#endif

#ifdef PROTEUS_ENABLE_HTTP
  std::cout << "HTTP server starting at port " << http_port << "\n";
  proteus::http::start(http_port);
#else
  while (1) {
    std::this_thread::yield();
  }
#endif

  proteus::terminate();

  return 0;
}
