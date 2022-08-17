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

#ifndef GUARD_PROTEUS_SERVERS_WEBSOCKET_SERVER
#define GUARD_PROTEUS_SERVERS_WEBSOCKET_SERVER

#include <drogon/HttpRequest.h>          // for HttpRequestPtr
#include <drogon/HttpTypes.h>            // for Get, WebSocketMessageType
#include <drogon/WebSocketConnection.h>  // for WebSocketConnectionPtr
#include <drogon/WebSocketController.h>  // for WS_PATH_ADD, WS_PATH_LIST...

#include <cstddef>    // for size_t
#include <memory>     // for shared_ptr, allocator
#include <stdexcept>  // for invalid_argument
#include <string>     // for string
#include <vector>     // for vector

#include "proteus/build_options.hpp"         // for PROTEUS_ENABLE_LOGGING
#include "proteus/core/interface.hpp"        // for Interface
#include "proteus/helpers/declarations.hpp"  // for BufferRawPtrs
#include "proteus/observation/logging.hpp"   // for LoggerPtr

namespace Json {
class Value;
}  // namespace Json
namespace proteus {
class InferenceRequest;
}  // namespace proteus

namespace proteus::http {

/**
 * @brief The DrogonWs Interface class encapsulates incoming requests from
 * Drogon's Websocket interface to the batcher.
 *
 */
class DrogonWs : public Interface {
 public:
  DrogonWs(const drogon::WebSocketConnectionPtr &conn,
           std::shared_ptr<Json::Value> json);

  std::shared_ptr<InferenceRequest> getRequest(
    const BufferRawPtrs &input_buffers, std::vector<size_t> &input_offsets,
    const BufferRawPtrs &output_buffers,
    std::vector<size_t> &output_offsets) override;

  size_t getInputSize() override;
  void errorHandler(const std::exception &e) override;

 private:
  void setJson();

  std::shared_ptr<Json::Value> json_;
  drogon::WebSocketConnectionPtr conn_;
};

/**
 * @brief The Websocket server handles incoming websocket requests. Currently,
 * this is primarily used to make inferences to video streaming workers.
 *
 */
class WebsocketServer
  : public drogon::WebSocketController<WebsocketServer, false> {
 public:
  WebsocketServer();  ///< constructor

  /**
   * @brief When a client sends a new message, this handler is invoked to parse
   * the request
   *
   * @param conn the websocket connection the client is using
   * @param message the message
   * @param type the type of message
   */
  void handleNewMessage(const drogon::WebSocketConnectionPtr &conn,
                        std::string &&message,
                        const drogon::WebSocketMessageType &type) override;
  /**
   * @brief When a client closes the connection, this handler is invoked.
   *
   * @param conn
   */
  void handleConnectionClosed(
    const drogon::WebSocketConnectionPtr &conn) override;
  /**
   * @brief When a client opens a connection, this handler is invoked.
   *
   * @param req
   * @param conn
   */
  void handleNewConnection(const drogon::HttpRequestPtr &req,
                           const drogon::WebSocketConnectionPtr &conn) override;
  WS_PATH_LIST_BEGIN
  WS_PATH_ADD("/models/infer", drogon::Get);
  WS_PATH_LIST_END

#ifdef PROTEUS_ENABLE_LOGGING
 private:
  Logger logger_{Loggers::kServer};
#endif
};

}  // namespace proteus::http

#endif  // GUARD_PROTEUS_SERVERS_WEBSOCKET_SERVER
