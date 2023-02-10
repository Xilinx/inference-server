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

#ifndef GUARD_AMDINFER_SERVERS_SERVER
#define GUARD_AMDINFER_SERVERS_SERVER

#include <cstdint>
#include <filesystem>
#include <memory>

namespace amdinfer {

class Server {
 public:
  /// Constructs a new Server object
  Server();
  /// Copy constructor
  Server(Server const&) = delete;
  /// Copy assignment constructor
  Server& operator=(const Server&) = delete;
  /// Move constructor
  Server(Server&& other) = default;
  /// Move assignment constructor
  Server& operator=(Server&& other) = default;
  /// Destructor
  ~Server();

  /**
   * @brief Start the HTTP server
   *
   * @param port port to use for the HTTP server
   */
  void startHttp(uint16_t port) const;
  /// Stop the HTTP server
  void stopHttp() const;
  /**
   * @brief Start the gRPC server
   *
   * @param port port to use for the gRPC server
   */
  void startGrpc(uint16_t port) const;
  /// Stop the gRPC server
  void stopGrpc() const;

  /**
   * @brief Set the path to the model repository associated with this server
   *
   * @param path path to the model repository
   * @param load_existing load all existing models found at the path
   */
  void setModelRepository(const std::filesystem::path& repository_path,
                          bool load_existing);
  /**
   * @brief Turn on active monitoring of the model repository path for new
   * files. A model repository must be set with setModelRepository() before
   * calling this method.
   *
   * @param use_polling set to true to use polling to check the directory for
   * new files, false to use events. Note that events may not work well on all
   * platforms.
   */
  void enableRepositoryMonitoring(bool use_polling);

  friend class NativeClient;

 private:
  struct ServerImpl;
  std::unique_ptr<ServerImpl> impl_;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_SERVERS_SERVER
