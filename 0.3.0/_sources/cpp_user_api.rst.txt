..
    Copyright 2021 Xilinx, Inc.
    Copyright 2022, Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

C++
===

Clients
-------

.. doxygenfunction:: amdinfer::serverHasExtension

.. doxygenfunction:: amdinfer::waitUntilServerReady

.. doxygenfunction:: amdinfer::waitUntilModelReady

.. doxygenfunction:: amdinfer::inferAsyncOrdered

.. doxygenfunction:: amdinfer::inferAsyncOrderedBatched

gRPC
^^^^

.. _user_cpp_clients_grpc:
.. doxygenclass:: amdinfer::GrpcClient
    :members:

HTTP
^^^^

.. _user_cpp_clients_http:
.. doxygenclass:: amdinfer::HttpClient
    :members:


Native
^^^^^^

.. _user_cpp_clients_native:
.. doxygenclass:: amdinfer::NativeClient
    :members:

WebSocket
^^^^^^^^^

.. _user_cpp_clients_websocket:
.. doxygenclass:: amdinfer::WebSocketClient
    :members:

Core
----

DataType
^^^^^^^^

.. _user_cpp_core_datatype:
.. doxygenclass:: amdinfer::DataType
    :members:

Exceptions
^^^^^^^^^^

.. _user_cpp_core_exceptions:
.. doxygenfile:: include/amdinfer/core/exceptions.hpp

Prediction
^^^^^^^^^^

.. _user_cpp_core_request_parameters:
.. doxygenclass:: amdinfer::RequestParameters
    :members:

.. _user_cpp_core_server_metadata:
.. doxygenstruct:: amdinfer::ServerMetadata
    :members:

.. _user_cpp_core_inference_request_input:
.. doxygenclass:: amdinfer::InferenceRequestInput
    :members:

.. _user_cpp_core_inference_request_output:
.. doxygenclass:: amdinfer::InferenceRequestOutput
    :members:

.. _user_cpp_core_inference_response:
.. doxygenclass:: amdinfer::InferenceResponse
    :members:

.. _user_cpp_core_inference_request:
.. doxygenclass:: amdinfer::InferenceRequest
    :members:

.. _user_cpp_core_inference_model_metadata_tensor:
.. doxygenclass:: amdinfer::ModelMetadataTensor
    :members:

.. _user_cpp_core_inference_model_metadata:
.. doxygenclass:: amdinfer::ModelMetadata
    :members:

Servers
-------

.. _user_cpp_servers_server:
.. doxygenclass:: amdinfer::Server
    :members:
