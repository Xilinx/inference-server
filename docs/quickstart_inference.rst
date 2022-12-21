..
    Copyright 2022 Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

Quickstart - Inference
======================

This quickstart is intended for a user who is not configuring or maintaining the inference server and is just making inference requests to an existing server.

You need the address of the server and the endpoint(s) for the models you want to use for inference.
Depending on how it is configured, the server may support HTTP/REST, gRPC or both protocols.
Your server administrator can provide this information.

Once the server is running somewhere, you can communicate with it from your own machine as long as you can connect to it.

Making requests with the library
--------------------------------

The easiest way to make requests is using the AMD Inference Server's library ``amdinfer``.
The library allows you to make a client object that you can use to communicate with the server over any protocol that the server supports.
Clients have the same base set of methods so you can easily replace one with another.
It also allows you to make objects to hold the requests and responses and use defined methods for interacting with them.

The library is natively in C++ and has Python bindings that you can use as well.
The C++ version of the library is most easily available in the development container for the Inference Server as that has all the needed dependencies already.
If you want to use the C++ library outside the container, you need to resolve the dependencies yourself.
You can :ref:`install the Python version <python:Install the Python library>` on a host, in an environment or in a container using a package.

.. tabs::

    .. code-tab:: c++

        // your server administrator must provide the values for these variables:
        //   - http_server_addr: HTTP address of the server, if supported
        //   - grpc_server_addr: gRPC address of the server, if supported
        //   - endpoint: string to identify the model for inference. If there are
        //               multiple models available, each model will have its own
        //               endpoint that you can use to request inferences from it
        const std::string http_server_addr = "http://127.0.0.1:8998";
        const std::string grpc_server_addr = "127.0.0.1:50051";
        const std::string endpoint = "endpoint";

        #include "amdinfer/amdinfer.hpp"

        # create a client to communicate to the server over HTTP
        const amdinfer::HttpClient http_client{http_server_addr};

        # create a client to communicate to the server over gRPC
        const amdinfer::GrpcClient grpc_client{grpc_server_addr};

    .. code-tab:: python

        # your server administrator must provide the values for these variables:
        #   - http_server_addr: HTTP address of the server, if supported
        #   - grpc_server_addr: gRPC address of the server, if supported
        #   - endpoint: string to identify the model for inference. If there are
        #               multiple models available, each model will have its own
        #               endpoint that you can use to request inferences from it
        http_server_addr = "http://127.0.0.1:8998"
        grpc_server_addr = "127.0.0.1:50051"
        endpoint = "endpoint"

        import amdinfer

        # create a client to communicate to the server over HTTP
        http_client = amdinfer.HttpClient(http_server_addr)

        # create a client to communicate to the server over gRPC
        grpc_client = amdinfer.GrpcClient(grpc_server_addr)

The library also defines the objects that you have to populate to send a request: an ``InferenceRequest``.
A request is made up of, at minimum, one or more ``InferenceRequestInput`` objects that define the input tensor(s) of your request.
Each tensor must have a name, a data type, an associated shape and the data itself.

.. tabs::

    .. code-tab:: c++

        #include <vector>

        amdinfer::InferenceRequest request;

        amdinfer::InferenceRequestInput input_tensor;
        // depending on the implementation, the string used here may be significant
        input_tensor.setName("input_0");
        input_tensor.setDatatype(amdinfer::DataType::Int64);
        input_tensor.setShape({2, 3});
        // the data should be flattened
        std::vector<uint64_t> data{{1, 2, 3, 4, 5, 6}};
        input_tensor.setData(data.data());

        request.addInputTensor(input_tensor)

        response = http_client.modelInfer(endpoint, request)
        // either client can be used relatively interchangeably
        // response = grpc_client.modelInfer(endpoint, request)

    .. code-tab:: python

        request = amdinfer.InferenceRequest()

        input_tensor = amdinfer.InferenceRequestInput()
        # depending on the implementation, the string used here may be significant
        input_tensor.name = "input_0"
        input_tensor.datatype = amdinfer.DataType.INT64
        input_tensor.shape = [2, 3]
        # the data should be flattened
        input_tensor.setInt64Data([1, 2, 3, 4, 5, 6])

        request.addInputTensor(input_tensor)

        response = http_client.modelInfer(endpoint, request)
        # either client can be used relatively interchangeably
        # response = grpc_client.modelInfer(endpoint, request)

The result of the inference is an ``InferenceResponse`` object that you can examine to get the results.

For more information about these objects and the available methods, look at the examples or the documentation for the :ref:`C++ <cpp_user_api:c++>` and :ref:`Python <python:API>` APIs.

Making requests directly
------------------------

If you are unable to use the library, you can also communicate with the server directly over REST or gRPC.
However, you will need to construct the requests yourself in the correct format.
Both the :ref:`REST API <rest:rest endpoints>` and the :amdinferblob:`gRPC API <src/amdinfer/core/predict_api.proto>` are documented.
