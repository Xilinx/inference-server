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

from .exceptions import *
from .predict_api import (
    Datatype,
    RequestInput,
    RequestOutput,
    InferenceRequest,
    NumericalInferenceRequest,
    ImageInferenceRequest,
    WebsocketInferenceRequest,
    InferenceResponse,
    ResponseOutput,
    ErrorResponse,
    HtmlResponse,
)
from .server import *
from .rest import Client as RestClient
from .websocket import Client as WebsocketClient
