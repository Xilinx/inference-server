# Copyright 2021 Xilinx Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from enum import Enum
import json
import struct
import websocket

from .predict_api import InferenceRequest
from . import exceptions


class WebsocketOpcodes(Enum):
    continuation = 0
    text = 1
    binary = 2
    close = 8
    ping = 9
    pong = 10


class Client:
    def __init__(self, address):
        self.ws_addr = "ws://" + address
        self.wss_addr = "wss://" + address

        self.ws = websocket.WebSocket()

    def is_connected(self):
        return self.ws.connected

    def connect(self, endpoint):
        url = self.ws_addr + endpoint
        try:
            self.ws.connect(url)
        except websocket.WebSocketBadStatusException:
            raise exceptions.ConnectionError(
                f"Connecting to {url} over WS failed. Bad status returned."
            )

    def infer(self, request):
        if not self.ws.connected:
            self.connect("/models/infer")
        if isinstance(request, InferenceRequest):
            request = request.asdict()
        self.ws.send(json.dumps(request))

    def recv(self):
        if self.ws.connected:
            resp_opcode, msg = self.ws.recv_data()
            # https://websocket-client.readthedocs.io/en/latest/examples.html#receiving-connection-close-status-codes
            if resp_opcode == WebsocketOpcodes.close.value:
                return int(struct.unpack("!H", msg[0:2])[0])
            elif resp_opcode == WebsocketOpcodes.text.value:
                return json.loads(msg)
            else:
                raise exceptions.BadResponseError("Unknown response type in websocket.")
        else:
            raise exceptions.ConnectionError(
                "Recv over websocket failed. Websocket not connected."
            )

    def close(self):
        try:
            self.ws.close()
        except websocket.WebSocketConnectionClosedException:
            pass
