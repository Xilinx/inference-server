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

#ifndef GUARD_AMDINFER_SERVERS_SERVER
#define GUARD_AMDINFER_SERVERS_SERVER

#include <cstdint>
#include <filesystem>
#include <memory>

namespace amdinfer {

class Server {
 public:
  Server();
  ~Server();

  void startHttp(uint16_t port) const;
  void stopHttp() const;
  void startGrpc(uint16_t port) const;
  void stopGrpc() const;

  void setModelRepository(const std::filesystem::path& path) const;
  void enableRepositoryMonitoring(bool use_polling) const;

 private:
  struct ServerImpl;
  std::unique_ptr<ServerImpl> impl_;
};

}  // namespace amdinfer

#endif  // GUARD_AMDINFER_SERVERS_SERVER