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

Quickstart - Deployment
=======================

This quickstart is intended for a user who is deploying the inference server and configuring the available models for inference.

There are a few different methods to deploy the server and make it available for remote clients:

* Deploy on Docker with a development or deployment image
* Deploy on KServe with a deployment image
* Deploy without Docker

Once the server is up, you and any clients can :ref:`make requests to it <quickstart_inference:Quickstart - Inference>`.

Deployment vs. Development Images
---------------------------------

There are two kinds of Docker images you can make for the inference server: deployment and development.
The deployment image is optimized for size and only contains the runtime dependencies of the server to allow for quicker deployments.
It has limited debugging capabilities and it contains a precompiled binary for the server that automatically starts when the container starts.
In contrast, the development image is far larger in size because it also contains the build-time dependencies of the server.
It allows you to compile the server and build the `amdinfer` library.
You can build either the :ref:`deployment <docker:Build the production Docker image>` or development image using a script.

Deploying on Docker
-------------------

You can use either the deployment or development image to make the server available for inference with Docker.
This approach has the fewest external dependencies but requires more manual work to set up.

Deployment image
^^^^^^^^^^^^^^^^

Once you have the basic deployment image, you'll need to prepare the image by adding models that you want to support.
Follow these :ref:`instructions <docker:Prepare the image for Docker deployment>` for more information.

Development image
^^^^^^^^^^^^^^^^^

Once you have the development image, you can start the container, compile the server, and start it.
Follow these :ref:`instructions <quickstart_development:Compiling the AMD Inference Server>` for more information.

Deploying on KServe
-------------------

You can use `KServe <https://kserve.github.io/website/0.9/>`__ to deploy the inference server on a Kubernetes cluster.
Once KServe is installed on your Kubernetes cluster, you can use a prepared deployment image to make models available for inference.
Follow these :ref:`instructions <kserve:KServe>` for more information.

Deploying without Docker
------------------------

While deploying without Docker is not an officially supported approach, it is possible to make the server available without Docker.
The server is compiled with CMake so you can install all the dependencies on your host machine so that the CMake build can compile the server executable.
Looking at the :ref:`dependencies <dependencies:Dependencies>` can help you identify what third-party packages are needed by the server.

Loading models
--------------

Once the server is up using one of the approaches above, you must load the models you want to make available for inference.
The easiest way to load models is using the AMD Inference Server's library ``amdinfer``.
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

The library also defines the ``workerLoad`` API that you can use to load models.

.. note::

    The similar ``modelLoad`` API works in a related but different way.
    It is primarily meant for use by KServe.

.. tabs::

    .. code-tab:: c++

        amdinfer::RequestParameters parameters;

        parameters.put("model", "/path/to/model")
        parameters.put("batch", 8)

        // the first argument should be the worker
        const auto endpoint = client.workerLoad("migraphx", &parameters)

        amdinfer::waitUntilModelReady(client, endpoint)

    .. code-tab:: python

        parameters = amdinfer.RequestParameters()

        parameters.put("model", "/path/to/model")
        parameters.put("batch", 8)

        # the first argument should be the worker
        endpoint = client.workerLoad("migraphx", parameters)

        amdinfer.waitUntilModelReady(client, endpoint)

After the model is loaded and ready, it is ready to accept inferences at the given endpoint.

For more information about these objects and the available methods, look at the examples or the documentation for the :ref:`C++ <cpp_user_api:c++>` and :ref:`Python <python:API>` APIs.
