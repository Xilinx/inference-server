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

.. include:: links.rst

Quickstart - Inference
======================

This quickstart is intended for a user who is not configuring or maintaining the inference server and is just making inference requests to an existing server.
There are multiple ways to make requests to the server but this quickstart only covers making requests using the inference server's client library ``amdinfer``.

To make requests, you need the address of the server and the endpoint(s) for the models you want to use for inference.
Depending on how it is configured, the server may support HTTP/REST, gRPC or both protocols.
Your server administrator can provide this information.

Get the library
---------------

The ``amdinfer`` library allows you to make clients that you can use to communicate with the server over any protocol that the server supports.
Clients for different protocols have the same base set of methods so you can easily replace one with another.
The library can be used from C++ or Python.

To use the Python library, install with ``pip``:

.. code-block:: console

    $ pip install amdinfer

You can use the development container to use both the C++ and Python libraries:

.. code-block:: console

    $ git clone https://github.com/Xilinx/inference-server.git
    $ cd inference-server
    $ python3 docker/generate.py
    $ ./amdinfer dockerize
    $ ./amdinfer run --dev --net-host

This will build, start the development container and drop you into a terminal in the container.
Then, inside the container:

.. code-block:: console

    $ amdinfer install

This will install both the C++ and Python libraries in the container, which you can confirm:

.. tabs::

    .. code-tab:: console Python

        $ pip list | grep amdinfer

    .. code-tab:: console C++

        $ echo -e '#include "amdinfer/amdinfer.hpp"\nint main(){return 0;}' | g++ -x c++ -std=c++17 -o test.out /dev/stdin

Running the examples
--------------------

The `AMD Inference Server repository <InferenceServerRepository_>`_ includes examples that demonstrate running an end-to-end inference request.
To run the examples listed here, you need to ensure that the ``amdinfer`` Python library is installed and that the server you're using has a compatible model loaded.
These examples assume a ResNet50 model trained on the ImageNet dataset is available on the server and use a sample image and labels from the inference server repository but you can also use your own.

.. parsed-literal::

    $ wget :amdinferRawFull:`examples/resnet50/imagenet_classes.txt`
    $ wget :amdinferRawFull:`tests/assets/dog-3619020_640.jpg`
    $ wget :amdinferRawFull:`examples/resnet50/resnet.py`

.. tabs::

    .. group-tab:: CPU

        .. parsed-literal::

            $ wget :amdinferRawFull:`examples/resnet50/tfzendnn.py`
            $ python3 tfzendnn.py --ip <ip_address> --grpc-port <port> --endpoint <endpoint> --image ./dog-3619020_640.jpg --labels ./imagenet_classes.txt

    .. group-tab:: GPU

        .. parsed-literal::

            $ wget :amdinferRawFull:`examples/resnet50/migraphx.py`
            $ python3 migraphx.py --ip <ip_address> --http-port <port> --endpoint <endpoint> --image ./dog-3619020_640.jpg --labels ./imagenet_classes.txt

    .. group-tab:: FPGA

        .. parsed-literal::

            $ wget :amdinferRawFull:`examples/resnet50/vitis.py`
            $ python3 vitis.py --ip <ip_address> --http-port <port> --endpoint <endpoint> --image ./dog-3619020_640.jpg --labels ./imagenet_classes.txt

Using the library
-----------------

The examples above demonstrate a full end-to-end inference using the ``amdinfer`` Python library on a specific ResNet50 model.
You can write your own scripts and programs to make inference requests to other models.
The examples work similarly and you can use them for reference.

The first step is to create a client.
The type of client you create will depend on what protocols the server you're using supports and which protocol you want to use.

.. tabs::

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

        // create a client to communicate to the server over HTTP
        const amdinfer::HttpClient http_client{http_server_addr};

        // create a client to communicate to the server over gRPC
        const amdinfer::GrpcClient grpc_client{grpc_server_addr};

The library also defines the object that you have to populate to send a request: an ``InferenceRequest``.
A request is made up of, at minimum, one or more ``InferenceRequestInput`` objects that define the input tensor(s) of your request.
Each tensor must have a name, a data type, an associated shape and the data itself.

.. tabs::

    .. code-tab:: python

        request = amdinfer.InferenceRequest()

        input_tensor = amdinfer.InferenceRequestInput()
        # depending on the model, the string used here may be significant
        input_tensor.name = "input_0"
        input_tensor.datatype = amdinfer.DataType.INT64
        input_tensor.shape = [2, 3]
        # the data should be flattened
        input_tensor.setInt64Data([1, 2, 3, 4, 5, 6])

        request.addInputTensor(input_tensor)

        response = http_client.modelInfer(endpoint, request)
        # either client can be used interchangeably
        # response = grpc_client.modelInfer(endpoint, request)

    .. code-tab:: c++

        amdinfer::InferenceRequest request;

        amdinfer::InferenceRequestInput input_tensor;
        // depending on the endpoint, the string used here may be significant
        input_tensor.setName("input_0");
        input_tensor.setDatatype(amdinfer::DataType::Int64);
        input_tensor.setShape({2, 3});
        // the data should be flattened
        std::vector<uint64_t> data{{1, 2, 3, 4, 5, 6}};
        input_tensor.setData(data.data());

        request.addInputTensor(input_tensor)

        response = http_client.modelInfer(endpoint, request)
        // either client can be used interchangeably
        // response = grpc_client.modelInfer(endpoint, request)

The result of the inference is an ``InferenceResponse`` object that you can examine to get the results.

.. tabs::

    .. code-tab:: python

        assert not response.isError()
        output_tensors = response.getOutputs()

        for output_tensor in output_tensors:
            shape = output_tensor.shape
            datatype = output_tensor.datatype
            data = output.getFp32Data()
            size = output.getSize()

    .. code-tab:: c++

        assert(!response.isError());
        // vector of amdinfer::InferenceResponseOutput objects
        auto output_tensors = response.getOutputs();

        for(auto &output_tensor : outputs){
            auto shape = output_tensor.getShape();
            auto datatype = output_tensor.getDataType()
            auto* data = static_cast<float*>(output_tensor.getData());
            auto size = output.getSize();
        }

These examples show how to use the library to construct a request, make an inference and examine the response.
They do not show any model-specific pre- and post-processing that may be needed.
If needed, you must implement it or use an existing implementation for your model.

For more information about usage and the available methods, look at the examples or the documentation for the :ref:`C++ <cpp_user_api:c++>` and :ref:`Python <python:API>` APIs.
