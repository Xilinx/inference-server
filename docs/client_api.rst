..
    Copyright 2023 Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

Client API
==========

The client API enables users to interact with the inference server.
This page highlights some of the important methods that you can use.

Include the API
---------------

After installing the client API, you can include it in your code with a single line:

.. tabs::

    .. code-tab:: c++ C++

        #include "amdinfer/amdinfer.hpp"

    .. code-tab:: python Python

        import amdinfer

Create a client object
----------------------

A client object enables you to talk to the server using the protocol of your choice.
The inference server supports the following protocols which you can use independently.

.. tabs::

    .. code-tab:: c++ C++

        // native - the server must be started in the same process
        amdinfer::Server server;
        amdinfer::NativeClient client(&server);

        // HTTP/REST
        amdinfer::HttpClient client{"http://127.0.0.1:8998"};

        // gRPC
        amdinfer::GrpcClient client{"127.0.0.1:50051"};

    .. code-tab:: python Python

        # native - the server must be started in the same script
        server = amdinfer.Server()
        client = amdinfer.NativeClient(server)

        # HTTP/REST
        client = amdinfer.HttpClient("http://127.0.0.1:8998")

        # gRPC
        client = amdinfer.GrpcClient("127.0.0.1:50051")

All clients have the same interface after the initial construction so you can use any client in the next set of steps.

Server status
-------------

You can check the state and health of the server using the following methods of all client objects: ``serverMetadata()``, ``serverLive()``, ``serverReady()``, ``modelReady()``, ``modelMetadata()`` and ``hasHardware()``.

These base methods of client objects enable the following helper functions that take a client object as the first argument: ``serverHasExtension()``, ``waitUntilServerReady()``, ``waitUntilModelReady()`` and ``waitUntilModelNotReady()``.

You can see more information about these functions in the API documentation for :ref:`C++ <cpp_user_api:C++>` and :ref:`Python <python_api>`.

Loading a backend
-----------------

If the server you are using is not already ready to serve incoming inference requests for your model, you may need to load a :ref:`backend <backends:Backends>` to serve your model first.

Client objects provide two methods to load backends: ``modelLoad()`` and ``workerLoad()``.
The former loads the named model from the :ref:`model repository <model_repository:Model Repository>` while the latter is a lower-level method to directly load a backend with a path to a particular model file.
The path to the model file, and other load-time parameters, can be passed to the server with these methods.
Each backend defines its own load-time parameters so check the documentation for the backend you want to use.

When you load a backend, you get an *endpoint* that you can use to make further requests to.
For ``modelLoad()``, the endpoint is the same name as the model you pass to the method.
For ``workerLoad()``, the server will assign it an endpoint and return it from the call to ``workerLoad()``.

You can also load :ref:`ensembles <ensembles:Ensembles>` with ``loadEnsemble()``.

Making an inference request
---------------------------

A basic inference request to the server consists of a list of input tensors.
You can construct a request with something like this:

.. tabs::

    .. code-tab:: c++ C++

        amdinfer::InferenceRequest request;

        // void* data = ...;
        amdinfer::InferenceRequestInput input_tensor{
            data, {224, 224, 3}, amdinfer::DataType::FP32, "input"
        };
        request.addInputTensor(input_tensor);

        // add other tensors?

    .. code-tab:: python Python

        request = amdinfer.InferenceRequest()

        input_tensor = amdinfer.InferenceRequestInput()
        input_tensor.name = "input"
        input_tensor.datatype = amdinfer.DataType.FP32
        input_tensor.shape = (224, 224, 3)
        # data could be a list or a numpy array
        input_tensor.setFp32Data(data)
        request.addInputTensor(input_tensor)

        # add other tensors?

Once you have a request, you can use the client's ``modelInfer()`` method:

.. tabs::

    .. code-tab:: c++ C++

        // endpoint is a string from loading a backend or provided to you
        auto response = client.modelInfer(endpoint, request);

    .. code-tab:: python Python

        # endpoint is a string from loading a backend or provided to you
        response = client.modelInfer(endpoint, request)

The client API also provides other methods for making inferences such as ``modelInferAsync()`` and ``inferAsyncOrdered()``.
You can see more information about the available methods in the API documentation for :ref:`C++ <cpp_user_api:C++>` and :ref:`Python <python_api>`.

Parsing the response
--------------------

The basic response from the inference server consists of an array of output tensors.

.. tabs::

    .. code-tab:: c++ C++

        auto response = client.modelInfer(endpoint, request);
        if(!response.isError()){
            auto output_tensors = response.getOutputs();
            for(const auto& output_tensor : output_tensors){
                // you can use methods to get the shape, datatype and name and data
            }
        }


    .. code-tab:: python Python

        response = client.modelInfer(endpoint, request)
        if not response.isError():
            output_tensors = response.getOutputs()
            for output_tensor in output_tensors:
                # you can use methods to get the shape, datatype and name and data

You can see more information about the available methods in the API documentation for :ref:`C++ <cpp_user_api:C++>` and :ref:`Python <python_api>`.

Next steps
----------

Take a look at the examples to see these APIs used in practice.
