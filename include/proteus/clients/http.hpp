// Copyright 2022 Xilinx Inc.
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
 * @brief Defines the methods for interacting with Proteus with HTTP/REST
 */

#ifndef GUARD_PROTEUS_CLIENTS_HTTP
#define GUARD_PROTEUS_CLIENTS_HTTP

namespace proteus {

/**
 * @brief Start the HTTP server for collecting metrics. This is a no-op if
 * Proteus is compiled without HTTP support.
 *
 * @param port port to use
 */
void startHttpServer(int port);

/**
 * @brief Stop the HTTP server. This is a no-op if Proteus is compiled without
 * HTTP support.
 *
 */
void stopHttpServer();

}  // namespace proteus

#endif  // GUARD_PROTEUS_CLIENTS_HTTP
